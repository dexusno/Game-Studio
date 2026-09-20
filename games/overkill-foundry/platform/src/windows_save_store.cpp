#include "overkill/save_store.hpp"
#include <Windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace overkill {
namespace {
struct Incompatible : std::runtime_error { using std::runtime_error::runtime_error; };
struct File {
    HANDLE handle=INVALID_HANDLE_VALUE;
    explicit File(HANDLE value):handle(value){}
    ~File(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}
    File(const File&)=delete;File& operator=(const File&)=delete;
};
void require(bool value,const char* operation){if(!value)throw std::runtime_error(std::string(operation)+" (Windows error "+std::to_string(GetLastError())+")");}
std::filesystem::path sibling(const std::filesystem::path& path,const wchar_t* suffix){auto p=path;p+=suffix;return p;}
std::uint64_t hash(const std::string& data){std::uint64_t h=14695981039346656037ULL;for(unsigned char c:data){h^=c;h*=1099511628211ULL;}return h;}
void put(std::string& data,std::uint64_t value){for(int i=0;i<8;++i)data.push_back(static_cast<char>((value>>(i*8))&255));}
std::uint64_t get(const std::string& data,std::size_t& at){if(data.size()-at<8)throw std::runtime_error("Truncated save envelope.");std::uint64_t n=0;for(int i=0;i<8;++i)n|=static_cast<std::uint64_t>(static_cast<unsigned char>(data[at++]))<<(i*8);return n;}
std::string makeToken(){
    unsigned char bytes[16]{};
    const NTSTATUS status=BCryptGenRandom(nullptr,bytes,sizeof(bytes),BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if(status!=0)throw std::runtime_error("Cannot obtain unique save-commit identity.");
    static constexpr char digits[]="0123456789abcdef";std::string token;
    for(unsigned char b:bytes){token.push_back(digits[b>>4]);token.push_back(digits[b&15]);}return token;
}
std::uint64_t recordHash(const std::string& payload,std::uint64_t revision,const std::string& token){std::string body;put(body,revision);body+=token;body+=payload;return hash(body);}
std::string envelope(const std::string& payload,std::uint64_t revision,const std::string& token){std::string data="OFSAVE02";put(data,revision);put(data,payload.size());put(data,recordHash(payload,revision,token));data+=token;data+=payload;return data;}
StoredSave readOne(const std::filesystem::path& path){
    File file(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
    require(file.handle!=INVALID_HANDLE_VALUE,"Open save");
    LARGE_INTEGER size{};require(GetFileSizeEx(file.handle,&size)!=0,"Read save length");
    if(size.QuadPart<8 || static_cast<std::uint64_t>(size.QuadPart)>std::numeric_limits<std::size_t>::max())throw std::runtime_error("Invalid save envelope length.");
    std::string data(static_cast<std::size_t>(size.QuadPart),'\0');std::size_t offset=0;
    while(offset<data.size()){
        const DWORD wanted=static_cast<DWORD>(std::min<std::size_t>(data.size()-offset,1048576));DWORD received=0;
        require(ReadFile(file.handle,data.data()+offset,wanted,&received,nullptr)!=0 && received>0,"Read save");offset+=received;
    }
    if(data.compare(0,8,"OFSAVE02")!=0){if(data.compare(0,6,"OFSAVE")==0)throw Incompatible("Unsupported save envelope version; do not roll back.");throw std::runtime_error("Invalid save envelope signature.");}
    if(data.size()<64)throw std::runtime_error("Invalid version 2 save envelope length.");
    std::size_t at=8;const auto revision=get(data,at),length=get(data,at),expectedHash=get(data,at);
    const auto token=data.substr(at,32);at+=32;
    if(token.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::runtime_error("Invalid save-commit identity.");
    if(revision==0 || length!=data.size()-at)throw std::runtime_error("Invalid save revision or length.");
    auto payload=data.substr(at);if(recordHash(payload,revision,token)!=expectedHash)throw std::runtime_error("Save integrity check failed.");
    return {true,false,revision,std::move(payload),{},token,false};
}
bool filePresent(const std::filesystem::path& path){return GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES;}
void flush(const std::filesystem::path& path){
    File file(CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr));
    require(file.handle!=INVALID_HANDLE_VALUE,"Open committed save for flush");require(FlushFileBuffers(file.handle)!=0,"Flush committed save");
}
} // namespace
StoredSave SaveStore::read() const {
    std::string primaryError;
    try{return readOne(path_);}catch(const Incompatible& e){return {false,false,0,{},e.what()};}catch(const std::exception& e){primaryError=e.what();}
    try{auto old=readOne(sibling(path_,L".previous"));old.recoveredPrevious=true;old.error="Recovered last valid revision: "+primaryError;return old;}
    catch(const std::exception&){return {false,false,0,{},primaryError};}
}
StoredSave SaveStore::commit(const std::string& payload,const std::string& expectedToken,const std::function<void(SavePoint)>& probe) const {
    try{
        if(path_.empty() || path_.filename().empty())throw std::runtime_error("Save file path is empty.");
        if(!path_.parent_path().empty())std::filesystem::create_directories(path_.parent_path());
        // OS-owned handle lock survives neither a normal exit nor a crash; no stale-lock guessing.
        const auto lockPath=sibling(path_,L".lock");
        File lock(CreateFileW(lockPath.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
        require(lock.handle!=INVALID_HANDLE_VALUE,"Lock profile save");
        const bool hadCurrent=filePresent(path_),hadPrevious=filePresent(sibling(path_,L".previous"));
        const auto current=read();
        if((hadCurrent || hadPrevious) && !current.ok)throw std::runtime_error("Existing save is unreadable; refusing to replace it.");
        if((current.ok?current.token:std::string{})!=expectedToken)throw std::runtime_error("Save changed since it was loaded; reload before committing.");
        const auto expectedRevision=current.ok?current.revision:0;
        if(expectedRevision==std::numeric_limits<std::uint64_t>::max())throw std::runtime_error("Save revision exhausted.");
        const auto next=expectedRevision+1;const auto token=makeToken();const auto bytes=envelope(payload,next,token);const auto temporary=sibling(path_,L".temporary");
        {
            File file(CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));
            require(file.handle!=INVALID_HANDLE_VALUE,"Create temporary save");std::size_t at=0;
            while(at<bytes.size()){
                const DWORD amount=static_cast<DWORD>(std::min<std::size_t>(bytes.size()-at,1048576));DWORD written=0;
                require(WriteFile(file.handle,bytes.data()+at,amount,&written,nullptr)!=0 && written>0,"Write temporary save");at+=written;
            }
            if(probe)probe(SavePoint::TemporaryWritten);
            require(FlushFileBuffers(file.handle)!=0,"Flush temporary save");if(probe)probe(SavePoint::TemporaryFlushed);
        }
        const auto previous=sibling(path_,L".previous");
        if(current.recoveredPrevious){
            // Keep the verified backup intact; never promote the damaged primary to backup.
            require(MoveFileExW(temporary.c_str(),path_.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0,"Replace damaged primary save");
        }else if(hadCurrent){
            require(ReplaceFileW(path_.c_str(),temporary.c_str(),previous.c_str(),0,nullptr,nullptr)!=0,"Replace active save with backup");
        }else{
            require(MoveFileExW(temporary.c_str(),path_.c_str(),MOVEFILE_WRITE_THROUGH)!=0,"Commit first save");
        }
        if(probe)probe(SavePoint::Replaced);flush(path_);if(probe)probe(SavePoint::CommitFlushed);
        const auto verified=readOne(path_);if(verified.revision!=next || verified.token!=token || verified.payload!=payload)throw std::runtime_error("Committed save verification failed.");return verified;
    }catch(const std::exception& e){return {false,false,0,{},e.what(),{},true};}
}
} // namespace overkill
