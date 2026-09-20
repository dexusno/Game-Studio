#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace overkill {
// IO is outside the deterministic rules library. One committed envelope contains
// campaign, profile facts and receipts together; callers validate their schema.
enum class SavePoint { TemporaryWritten, TemporaryFlushed, Replaced, CommitFlushed };
struct StoredSave {
    bool ok=false, recoveredPrevious=false;
    std::uint64_t revision=0;
    std::string payload, error;
    std::string token;
    bool reconcileRequired=false;
};
class SaveStore {
public:
    explicit SaveStore(std::filesystem::path path): path_(std::move(path)) {}
    StoredSave read() const;
    // Compare the token returned by read/commit, or empty for a new save. Tokens
    // stay distinct even when recovery reuses a numeric revision or payload.
    // UI success must follow ok=true. On failure, read again and reconcile the
    // payload's operation receipts before retry: replacement may have succeeded.
    StoredSave commit(const std::string& payload, const std::string& expectedToken,
                      const std::function<void(SavePoint)>& faultProbe={}) const;
private:
    std::filesystem::path path_;
};
} // namespace overkill
