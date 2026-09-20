#pragma once
#include <algorithm>
#include <cmath>
#include <optional>

namespace foundry::presentation {
// Initial presentation tuning, not a second collection/economy implementation.
// Only the measured miss/good/perfect category crosses into the shared core.
inline constexpr const char* PrecisionVersion = "precision-ui-0.1";
inline constexpr double PrecisionSeconds = 1.8;
enum class PrecisionStage { Ready, Running, Resolved };

class PrecisionAttempt {
public:
    explicit PrecisionAttempt(int widthPercent = 100)
        : width_(std::clamp(widthPercent, 1, 400) / 100.0) {}
    PrecisionStage stage() const { return stage_; }
    double position() const { return std::clamp(elapsed_ / PrecisionSeconds, 0.0, 1.0); }
    double goodHalfWidth() const { return std::min(.49, .16 * width_); }
    double perfectHalfWidth() const { return std::min(goodHalfWidth(), .045 * width_); }
    double elapsed() const { return elapsed_; }
    bool paused() const { return stage_ == PrecisionStage::Running && !focused_; }
    bool start() {
        if (stage_ != PrecisionStage::Ready) return false;
        stage_ = PrecisionStage::Running;
        return true;
    }
    int score(double marker) const {
        if (!std::isfinite(marker) || marker < 0 || marker > 1) return 0;
        if (marker >= .5 - perfectHalfWidth() && marker <= .5 + perfectHalfWidth()) return 2;
        if (marker >= .5 - goodHalfWidth() && marker <= .5 + goodHalfWidth()) return 1;
        return 0;
    }
    std::optional<int> stop() {
        if (stage_ != PrecisionStage::Running) return std::nullopt;
        stage_ = PrecisionStage::Resolved;
        return score(position());
    }
    std::optional<int> tick(double seconds, bool focused) {
        if (stage_ != PrecisionStage::Running) return std::nullopt;
        const bool wasFocused = focused_;
        focused_ = focused;
        // The first frame back may contain time spent outside the game. Do
        // not charge it. Losing focus never submits a collection or outcome.
        if (!focused || !wasFocused || !std::isfinite(seconds) || seconds < 0) return std::nullopt;
        elapsed_ = std::min(PrecisionSeconds, elapsed_ + seconds);
        if (elapsed_ >= PrecisionSeconds) {
            stage_ = PrecisionStage::Resolved;
            return 0;
        }
        return std::nullopt;
    }
private:
    double width_, elapsed_ = 0;
    bool focused_ = true;
    PrecisionStage stage_ = PrecisionStage::Ready;
};
} // namespace foundry::presentation
