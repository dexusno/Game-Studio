#include <Windows.h>
#include "overkill/save_store.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
namespace {
void check(bool ok, const std::string& message) { if (!ok) throw std::runtime_error(message); }
void write(const fs::path& path, const std::string& bytes) { std::ofstream f(path, std::ios::binary | std::ios::trunc); f.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); check(static_cast<bool>(f), "probe could not write bytes"); }
std::string bytes(const fs::path& path) { std::ifstream f(path, std::ios::binary); check(static_cast<bool>(f), "probe could not read bytes"); return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()}; }
fs::path suffix(fs::path path, const wchar_t* text) { path += text; return path; }
overkill::StoredSave putCurrent(overkill::SaveStore& store, const std::string& payload, const std::function<void(overkill::SavePoint)>& probe = {}) { return store.commit(payload, store.read().token, probe); }
void seed(const fs::path& path) { overkill::SaveStore s(path); check(s.commit("A", "").ok, "seed A"); check(putCurrent(s, "B").ok, "seed B"); }
void crash(const fs::path& path, std::uint64_t revision, int point) {
    std::vector<wchar_t> exe(32768); const DWORD n = GetModuleFileNameW(nullptr, exe.data(), static_cast<DWORD>(exe.size())); check(n > 0 && n < exe.size(), "GetModuleFileNameW");
    std::wstring command = L"\"" + std::wstring(exe.data(), n) + L"\" --crash \"" + path.wstring() + L"\" " + std::to_wstring(revision) + L" " + std::to_wstring(point);
    STARTUPINFOW startup{}; startup.cb = sizeof(startup); PROCESS_INFORMATION process{};
    check(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process) != 0, "CreateProcessW");
    const auto wait = WaitForSingleObject(process.hProcess, 10000); DWORD code = 0; GetExitCodeProcess(process.hProcess, &code); CloseHandle(process.hThread); CloseHandle(process.hProcess);
    check(wait == WAIT_OBJECT_0 && code == 75, "crash probe boundary not reached");
}
struct Handle { HANDLE value; ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); } };
}
int wmain(int argc, wchar_t** argv) {
    if (argc == 5 && std::wstring(argv[1]) == L"--crash") {
        overkill::SaveStore s(argv[2]); const int point = std::stoi(argv[4]);
        const auto entry = s.read(); if (entry.revision != std::stoull(argv[3])) return 4;
        const auto r = s.commit("C", entry.token, [point](overkill::SavePoint p) { if (static_cast<int>(p) == point) ExitProcess(75); });
        return r.ok ? 0 : 3;
    }
    if (argc != 2) return 2;
    const auto root = fs::path(argv[1]) / std::to_string(GetCurrentProcessId()); fs::create_directories(root);
    int passed = 0, failed = 0;
    const auto test = [&](const char* name, auto body) {
        try { body(root / (std::string(name) + ".ofsave")); ++passed; std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { ++failed; std::cout << "FAIL " << name << ": " << e.what() << '\n'; }
    };
    test("ordinary_stale_revision", [](const fs::path& p) { overkill::SaveStore s(p); const auto first = s.commit("A", ""); check(first.ok && s.commit("B", first.token).ok, "seed"); check(!s.commit("stale", first.token).ok && s.read().payload == "B", "stale state overwrote B"); });
    test("revision_reuse_after_recovery", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); const auto stale = s.read(); write(p, "damaged primary"); const auto recovered = s.read();
        check(recovered.ok && recovered.recoveredPrevious && recovered.revision == 1, "expected backup A"); const auto repair = s.commit("repair", recovered.token); check(repair.ok && repair.revision == stale.revision && repair.token != stale.token, "repair identity failed");
        const auto result = s.commit("stale B with a later purchase", stale.token);
        check(!result.ok && s.read().payload == "repair", "stale B revision 2 accepted after repair reused revision 2; repaired state overwritten at revision " + std::to_string(s.read().revision));
    });
    test("identical_payload_repair_identity", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); const auto stale = s.read(); write(p, "damaged primary"); const auto recovered = s.read(); const auto repair = s.commit(stale.payload, recovered.token);
        check(repair.ok && repair.revision == stale.revision && repair.payload == stale.payload && repair.token != stale.token, "identical payload repair reused commit identity");
        const auto rejected = s.commit("stale write", stale.token); check(!rejected.ok && rejected.reconcileRequired && s.read().token == repair.token && s.read().payload == "B", "old token accepted after identical payload repair");
    });
    test("last_valid_backup_preserved", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); const auto backup = bytes(suffix(p, L".previous")); write(p, "bad"); check(putCurrent(s, "repair").ok, "repair");
        check(bytes(suffix(p, L".previous")) == backup, "repair replaced last valid backup"); write(p, "bad again"); const auto r = s.read(); check(r.ok && r.payload == "A", "second damage lost original valid backup");
    });
    test("unknown_envelope_version", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); auto future = bytes(p); future[7] = '3'; write(p, future); check(!s.read().ok, "silently rolled back unsupported primary");
        check(!s.commit("replacement", "").ok && bytes(p) == future, "overwrote incompatible save");
    });
    test("legacy_short_envelope", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p);
        // Exact valid v1 A/revision-1 envelope retained from the initial review.
        const unsigned char legacy[] = {0x4f,0x46,0x53,0x41,0x56,0x45,0x30,0x31,0x01,0,0,0,0,0,0,0,0x01,0,0,0,0,0,0,0,0x1f,0xa2,0xf5,0x8f,0xdc,0x6d,0x9a,0x52,0x41};
        const std::string original(reinterpret_cast<const char*>(legacy), sizeof(legacy)); write(p, original); const auto r = s.read();
        check(!r.ok && r.error.find("Unsupported") != std::string::npos, "valid 33-byte OFSAVE01 primary silently recovered a v2 backup instead of reporting incompatibility");
        check(!s.commit("replacement", "").ok && bytes(p) == original, "replaced incompatible short legacy primary");
    });
    test("corruption_variants", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); const auto good = bytes(p);
        for (std::size_t offset : {std::size_t(8), std::size_t(16), std::size_t(24), std::size_t(32), good.size() - 1}) { auto corrupt = good; corrupt[offset] ^= 1; write(p, corrupt); const auto r = s.read(); check(r.ok && r.recoveredPrevious && r.payload == "A", "corrupt revision/length/checksum/token/payload not recovered"); }
        write(p, good.substr(0, 31)); check(s.read().ok && s.read().recoveredPrevious, "truncation not recovered");
        write(suffix(p, L".previous"), "also corrupt"); const auto damaged = bytes(p); check(!s.read().ok && !s.commit("C", "").ok && bytes(p) == damaged, "both-invalid saves were overwritten");
    });
    test("lock_contention_and_release", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); const auto before = bytes(p);
        { Handle h{CreateFileW(suffix(p, L".lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)}; check(h.value != INVALID_HANDLE_VALUE, "probe lock open"); check(!putCurrent(s, "C").ok && bytes(p) == before, "commit did not fail safely while locked"); }
        check(putCurrent(s, "C").ok, "persistent lock file prevents next commit after handle close");
    });
    test("temporary_open_failure", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); const auto before = bytes(p);
        { Handle h{CreateFileW(suffix(p, L".temporary").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)}; check(h.value != INVALID_HANDLE_VALUE, "probe temporary open"); check(!putCurrent(s, "C").ok && bytes(p) == before, "temporary failure changed primary"); }
        check(putCurrent(s, "C").ok, "retry after temporary failure");
    });
    test("replacement_failure", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); const auto before = bytes(p);
        { Handle h{CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)}; check(h.value != INVALID_HANDLE_VALUE, "probe primary open"); check(!putCurrent(s, "C").ok && bytes(p) == before, "failed replacement changed primary"); }
        check(putCurrent(s, "C").ok, "retry after replacement failure");
    });
    test("post_replacement_flush_failure", [](const fs::path& p) {
        seed(p); overkill::SaveStore s(p); Handle h{INVALID_HANDLE_VALUE};
        const auto result = putCurrent(s, "C", [&](overkill::SavePoint point) { if (point == overkill::SavePoint::Replaced) { h.value = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); check(h.value != INVALID_HANDLE_VALUE, "probe handle open after replace"); } });
        check(!result.ok && result.reconcileRequired && s.read().ok && s.read().payload == "C" && s.read().revision == 3, "expected observable committed state and reconciliation flag despite flush failure");
        std::cout << "NOTE failed commit can be present on disk: " << result.error << '\n';
    });
    test("exception_boundaries", [](const fs::path& p) {
        for (int point = 0; point < 4; ++point) { const auto q = suffix(p, std::to_wstring(point).c_str()); seed(q); overkill::SaveStore s(q); const auto r = putCurrent(s, "C", [point](overkill::SavePoint at) { if (static_cast<int>(at) == point) throw std::runtime_error("injected"); }); check(!r.ok && r.reconcileRequired, "injection not reached or missing reconciliation flag"); const auto loaded = s.read(); check(loaded.ok && loaded.payload == (point < 2 ? "B" : "C"), "partial transaction after exception"); check(s.commit("retry", loaded.token).ok, "lock retained after exception"); }
    });
    test("first_commit_process_exit", [](const fs::path& p) {
        for (int point = 0; point < 4; ++point) { const auto q = suffix(p, std::to_wstring(point).c_str()); overkill::SaveStore s(q); crash(q, 0, point); const auto r = s.read(); check(point < 2 ? !r.ok : (r.ok && r.payload == "C" && r.revision == 1), "wrong first-save crash result"); check(s.commit("retry", r.token).ok, "crash leaked OS lock or blocked abandoned-temp retry"); }
    });
    test("repair_process_exit", [](const fs::path& p) {
        for (int point = 0; point < 4; ++point) { const auto q = suffix(p, std::to_wstring(point).c_str()); seed(q); overkill::SaveStore s(q); const auto backup = bytes(suffix(q, L".previous")); write(q, "bad"); crash(q, 1, point); const auto r = s.read(); check(r.ok && r.payload == (point < 2 ? "A" : "C"), "wrong recovery crash result"); check(bytes(suffix(q, L".previous")) == backup, "crash repair changed verified backup"); check(s.commit("retry", r.token).ok, "crash leaked repair lock"); }
    });
    test("binary_and_multichunk", [](const fs::path& p) {
        overkill::SaveStore s(p); check(s.commit("", "").ok && s.read().payload.empty(), "empty payload"); std::string payload(2 * 1048576 + 73, '\0'); for (std::size_t i = 0; i < payload.size(); ++i) payload[i] = static_cast<char>(i % 256); check(putCurrent(s, payload).ok && s.read().payload == payload, "binary or multichunk mismatch");
    });
    std::cout << "RESULT " << passed << " passed; " << failed << " failed. Evidence: " << root.u8string() << '\n';
    return failed == 0 ? 0 : 1;
}
