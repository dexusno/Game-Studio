#include "FoundryProfiles.h"
#if FOUNDRY_WITH_CAMPAIGN
#include <algorithm>
#include <stdexcept>

namespace foundry_profiles {
namespace {
constexpr const char* NameHeader="FOUNDRY-PROFILE-NAME-1\n";
constexpr const char* SelectionHeader="FOUNDRY-PROFILE-SELECTION-1\n";
bool validId(const std::string& id){return id.size()==32&&id.find_first_not_of("0123456789abcdef")==std::string::npos;}
bool present(const std::filesystem::path& path){return std::filesystem::exists(path)||std::filesystem::exists(std::filesystem::path(path).concat(L".previous"));}
std::string trim(std::string text){
    const auto first=text.find_first_not_of(' ');if(first==std::string::npos)return {};
    return text.substr(first,text.find_last_not_of(' ')-first+1);
}
bool validName(const std::string& name){
    if(name.empty()||name.size()>160)return false;
    // Names never determine a filesystem path. Reject controls and malformed
    // UTF-8 so the same saved name renders consistently in either client.
    for(std::size_t i=0;i<name.size();){
        const auto c=static_cast<unsigned char>(name[i++]);
        if(c<0x20||c==0x7f)return false;
        if(c<0x80)continue;
        int count=0;std::uint32_t value=0,minimum=0;
        if(c>=0xc2&&c<=0xdf){count=1;value=c&0x1f;minimum=0x80;}
        else if(c>=0xe0&&c<=0xef){count=2;value=c&0x0f;minimum=0x800;}
        else if(c>=0xf0&&c<=0xf4){count=3;value=c&0x07;minimum=0x10000;}
        else return false;
        if(name.size()-i<static_cast<std::size_t>(count))return false;
        for(int j=0;j<count;++j){const auto next=static_cast<unsigned char>(name[i++]);if((next&0xc0)!=0x80)return false;value=(value<<6)|(next&0x3f);}
        if(value<minimum||value>0x10ffff||(value>=0x80&&value<=0x9f)||(value>=0xd800&&value<=0xdfff))return false;
    }
    return true;
}
std::string payloadValue(const std::string& payload,const char* header){
    const std::string prefix=header;
    if(payload.compare(0,prefix.size(),prefix)!=0)throw std::runtime_error("Unsupported profile metadata version.");
    return payload.substr(prefix.size());
}
Result writeMetadata(const std::filesystem::path& path,const std::string& payload,const std::string& token,const std::function<void(overkill::SavePoint)>& probe={}){
    overkill::SaveStore store(path);const auto result=store.commit(payload,token,probe);
    if(result.ok)return {true,{}};
    // Replacement can succeed before a reported I/O failure. Reconcile only
    // the exact desired metadata; never retry or weaken the storage contract.
    const auto latest=store.read();
    if(latest.ok&&latest.payload==payload)return {true,"Profile metadata recovered after saving."};
    return {false,result.error};
}
}
Store::Store(std::filesystem::path defaultPath,const overkill::CampaignRules& rules,bool fixedPath)
    :rules_(rules),defaultPath_(std::filesystem::absolute(std::move(defaultPath)).lexically_normal()),
     directory_(defaultPath_.parent_path()/L"Profiles"),path_(defaultPath_),fixed_(fixedPath){
    if(fixed_)activeName_="Inspection save";
    else try{
        const auto preference=directory_/L"selected.ofmeta";
        if(present(preference)){
            const auto saved=overkill::SaveStore(preference).read();
            if(!saved.ok)notice_="The last profile selection could not be restored. Choose a profile; its campaign files are unchanged.";
            else{
                const auto selected=payloadValue(saved.payload,SelectionHeader);
                const auto available=profiles();
                const auto found=std::find_if(available.begin(),available.end(),[&](const Profile& p){return p.id==selected;});
                if(found!=available.end()){activeId_=found->id;activeName_=found->name;path_=found->path;}
                else notice_="The last selected profile is unavailable. Choose a profile.";
                if(saved.recoveredPrevious)notice_="Recovered the previous profile selection. Campaign files are unchanged.";
            }
        }
    }catch(const std::exception& e){notice_=std::string("Profile selection could not be restored: ")+e.what();}
    session_=std::make_unique<overkill::CampaignSession>(path_,rules_);
}
bool Store::hasSave()const{return present(path_);}
std::filesystem::path Store::profileDirectory(const std::string& id)const{
    if(!validId(id))throw std::runtime_error("Invalid profile identity.");
    const auto root=std::filesystem::weakly_canonical(directory_);
    const auto candidate=root/std::filesystem::path(id);
    // Reject junction/symlink aliases as well as traversal: a profile must not
    // resolve to another profile or outside this profile directory.
    if(std::filesystem::weakly_canonical(candidate)!=candidate)throw std::runtime_error("Profile path is an alias; select an independent profile.");
    return candidate;
}
std::vector<Profile> Store::profiles()const{
    if(fixed_)return {{"fixed",activeName_,{},path_,hasSave()}};
    std::vector<Profile> result={{"default","Default",{},defaultPath_,present(defaultPath_)}};
    if(!std::filesystem::exists(directory_))return result;
    for(const auto& item:std::filesystem::directory_iterator(directory_)){
        const auto bytes=item.path().filename().u8string();
        const std::string id(reinterpret_cast<const char*>(bytes.data()),bytes.size());
        if(!validId(id)||!item.is_directory())continue;
        try{
            const auto folder=profileDirectory(id),path=folder/L"profile.ofsave",metadata=folder/L"name.ofmeta";
            if(!present(metadata)&&!present(path))continue;
            Profile profile{id,"Recovered profile "+id.substr(0,8),{},path,present(path)};
            const auto saved=overkill::SaveStore(metadata).read();
            if(saved.ok){
                try{const auto name=payloadValue(saved.payload,NameHeader);if(!validName(name))throw std::runtime_error("Invalid profile name.");profile.name=name;
                    if(saved.recoveredPrevious)profile.note="Recovered the previous profile name.";
                }catch(const std::exception&){profile.note="Profile name unavailable; campaign file retained.";}
            }else profile.note="Profile name unavailable; campaign file retained.";
            result.push_back(std::move(profile));
        }catch(const std::exception&){/* Do not follow aliased profile folders. */}
    }
    std::sort(result.begin()+1,result.end(),[](const Profile& a,const Profile& b){return a.name==b.name?a.id<b.id:a.name<b.name;});
    return result;
}
Result Store::create(const std::string& id,const std::string& requestedName,const std::function<void(overkill::SavePoint)>& probe){
    if(fixed_)return {false,"This launch uses a fixed save path."};
    const auto name=trim(requestedName);
    if(!validName(name))return {false,"Enter a short profile name without line breaks or control characters."};
    try{
        const auto folder=profileDirectory(id);
        if(std::filesystem::exists(folder))return {false,"That profile identity already exists. No files were replaced."};
        std::filesystem::create_directories(folder);
        return writeMetadata(folder/L"name.ofmeta",std::string(NameHeader)+name,{},probe);
    }catch(const std::exception& e){return {false,e.what()};}
}
Result Store::rememberSelection(const std::string& id)const{
    const auto path=directory_/L"selected.ofmeta";overkill::SaveStore disk(path);
    std::string token;
    if(present(path)){const auto saved=disk.read();if(!saved.ok)return {false,saved.error};token=saved.token;}
    return writeMetadata(path,std::string(SelectionHeader)+id,token);
}
Result Store::select(const std::string& id){
    if(fixed_)return {false,"This launch uses a fixed save path."};
    try{
        const auto available=profiles();const auto found=std::find_if(available.begin(),available.end(),[&](const Profile& p){return p.id==id;});
        if(found==available.end())return {false,"Profile not found. No campaign was changed."};
        auto candidate=std::make_unique<overkill::CampaignSession>(found->path,rules_);
        bool recovered=false;
        if(found->hasSave){const auto loaded=candidate->load();if(!loaded.ok)return {false,"Profile could not be opened: "+loaded.reason};recovered=loaded.recoveredPrevious;}
        session_=std::move(candidate);path_=found->path;activeId_=found->id;activeName_=found->name;
        const auto remembered=rememberSelection(id);
        notice_=remembered.ok?(recovered?"Recovered the previous valid campaign save.":"Profile selected. Choose New Game or Continue.")
            :"Profile selected, but the last-selection preference could not be saved. Campaign saving is separate.";
        return {true,notice_};
    }catch(const std::exception& e){return {false,e.what()};}
}
}
#endif
