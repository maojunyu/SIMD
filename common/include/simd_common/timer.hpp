#pragma once
#include <chrono>

namespace simd {

// 高精度计时器（RAII），基于 steady_clock
class Timer {
public:
    using Clock = std::chrono::steady_clock;

    Timer() : start_(Clock::now()) {}

    // 重置起点
    void reset() { start_ = Clock::now(); }

    // 从起点（或上次 reset）到现在经过的时间
    double elapsed_ns() const {
        return std::chrono::duration<double, std::nano>(Clock::now() - start_).count();
    }
    double elapsed_us() const { return elapsed_ns() / 1e3; }
    double elapsed_ms() const { return elapsed_ns() / 1e6; }

private:
    Clock::time_point start_;
};

} // namespace simd
