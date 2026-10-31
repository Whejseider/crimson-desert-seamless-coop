#pragma once
// Snapshot interpolation for remote players: render them slightly in the past (interp_delay) and blend
// between the two snapshots around that time. Header-only, pure.
#include <cmath>
#include <cstddef>
#include <deque>
#include <optional>

namespace cdcoop::net {

struct InterpState {
    double t = 0;  // seconds, receiver clock
    float pos[3] = {0, 0, 0};
    float yaw = 0;
};

inline float lerp_angle(float a, float b, float f) {
    constexpr float kPi = 3.14159265358979f;
    float d = std::fmod(b - a + kPi, 2 * kPi);
    if (d < 0) d += 2 * kPi;
    return a + (d - kPi) * f;
}

class InterpBuffer {
public:
    explicit InterpBuffer(double interp_delay_s = 0.1, std::size_t capacity = 32) : delay_(interp_delay_s), cap_(capacity) {}

    void push(const InterpState& s) {
        if (!buf_.empty() && s.t <= buf_.back().t) return;  // drop out-of-order / duplicate
        buf_.push_back(s);
        while (buf_.size() > cap_) buf_.pop_front();
    }

    std::optional<InterpState> sample(double now) const {
        if (buf_.empty()) return std::nullopt;
        const double rt = now - delay_;
        if (rt <= buf_.front().t) return buf_.front();
        for (std::size_t i = 1; i < buf_.size(); ++i) {
            const InterpState& a = buf_[i - 1];
            const InterpState& b = buf_[i];
            if (rt <= b.t) {
                const float f = static_cast<float>((rt - a.t) / (b.t - a.t));
                InterpState o;
                o.t = rt;
                for (int k = 0; k < 3; ++k) o.pos[k] = a.pos[k] + (b.pos[k] - a.pos[k]) * f;
                o.yaw = lerp_angle(a.yaw, b.yaw, f);
                return o;
            }
        }
        return buf_.back();  // no newer data: hold last (extrapolation is deliberately not done)
    }

    std::size_t size() const { return buf_.size(); }

private:
    double delay_;
    std::size_t cap_;
    std::deque<InterpState> buf_;
};

}  // namespace cdcoop::net
