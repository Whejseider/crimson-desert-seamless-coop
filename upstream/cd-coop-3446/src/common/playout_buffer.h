#pragma once
// Co-op+ (2026-11-06, low latency): how long after the fastest transit the other player's moves are drawn. The game
// draws a remote actor from a buffer of timed positions and never extrapolates, so a position must be in that buffer
// before its time comes, else the copy stalls and then jumps. The buffer therefore covers the moves' spacing (measured
// from their timestamps: ~25 ms for the game's own MoveActorReqs, one frame with the pose stream of local_capture.cpp)
// plus how late they arrive beyond the fastest transit (ClockMapper::lateness, the 95th percentile). It rises fast when
// the connection gets jumpier and falls slowly, 1 ms per event, so the timestamps we give the game never step
// backwards. Before: a fixed 150 ms on top of the game's own 200 ms.

#include <algorithm>
#include <cstdint>
#include <deque>
#include <vector>

namespace cdmp {

class PlayoutBuffer {
public:
    static constexpr int64_t kDefaultGapMs = 25;  // the game's MoveActorReq spacing, until moves are measured
    static constexpr int64_t kMaxGapMs = 60;      // longer pauses between moves are standing still, not spacing
    static constexpr size_t kGapSamples = 128;
    static constexpr int64_t kMaxMs = 250;
    static constexpr int64_t kRiseMs = 5;  // per event
    static constexpr int64_t kFallMs = 1;

    explicit PlayoutBuffer(int64_t min_ms) : min_ms_(std::clamp<int64_t>(min_ms, 0, kMaxMs)) {}

    // A move of the other player with this timestamp (their clock) arrived.
    void note_move(uint64_t ts) {
        if (last_move_ts_ && ts > last_move_ts_ && ts - last_move_ts_ <= static_cast<uint64_t>(kMaxGapMs)) {
            gaps_.push_back(static_cast<int64_t>(ts - last_move_ts_));
            if (gaps_.size() > kGapSamples) gaps_.pop_front();
        }
        if (ts > last_move_ts_) last_move_ts_ = ts;
    }
    // The spacing of the other player's moves (95th percentile of the recent ones: a longer gap than the buffer
    // stalls the copy until the next move).
    int64_t gap_ms() const {
        if (gaps_.size() < 8) return kDefaultGapMs;
        std::vector<int64_t> v(gaps_.begin(), gaps_.end());
        const size_t k = v.size() * 95 / 100;
        std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(k), v.end());
        return v[k];
    }

    // Co-op+: a predicted position halfway to the next move goes in after each move (CoopLink::predict_after), so
    // the buffer only has to cover half the spacing.
    void set_predicting(bool on) { predicting_ = on; }

    // `late_p95`: ClockMapper::lateness(95). Returns the buffer to use now.
    int64_t update(int64_t late_p95) {
        const int64_t gap = predicting_ ? (gap_ms() + 1) / 2 : gap_ms();
        const int64_t target = std::clamp(late_p95 + gap, min_ms_, kMaxMs);
        if (!started_) {
            ms_ = target;
            started_ = true;
        } else if (target > ms_) {
            ms_ = std::min(target, ms_ + kRiseMs);
        } else {
            ms_ = std::max(target, ms_ - kFallMs);
        }
        return ms_;
    }
    int64_t ms() const { return started_ ? ms_ : std::max(min_ms_, kDefaultGapMs); }
    int64_t min_ms() const { return min_ms_; }
    void reset() {
        started_ = false;
        gaps_.clear();
        last_move_ts_ = 0;
    }

private:
    int64_t min_ms_;
    int64_t ms_ = 0;
    bool started_ = false;
    bool predicting_ = false;
    std::deque<int64_t> gaps_;
    uint64_t last_move_ts_ = 0;
};

}  // namespace cdmp
