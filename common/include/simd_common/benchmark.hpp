#pragma once
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <string>

namespace simd {

// 阻止编译器把"无副作用"的计算优化掉（保证 benchmark 真实执行 fn）
template <typename T>
inline void do_not_optimize(T const& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(value) : "memory");
#else
    // 兜底：volatile 读，避免被删除
    volatile char sink = *reinterpret_cast<const volatile char*>(&value);
    (void)sink;
#endif
}

struct BenchResult {
    double ns_per_op = 0.0;      // 单次调用耗时（纳秒）
    double ops_per_sec = 0.0;    // 每秒操作数
    double gbytes_per_sec = 0.0; // 带宽（若传入 bytes 参数）
    std::size_t iters = 0;
};

// 基准测试：
//   fn       —— 无参可调用对象，返回任意值；每次调用计为 1 次 op
//   iters    —— 计时循环次数
//   warmup   —— 预热次数（先跑几轮让缓存/频率稳定）
//   bytes    —— 单次 op 处理的数据量（字节），用于算带宽
template <typename F>
BenchResult benchmark(F&& fn, std::size_t iters = 1'000'000,
                      std::size_t warmup = 1'000, double bytes = 0.0) {
    for (std::size_t i = 0; i < warmup; ++i) {
        do_not_optimize(fn());
    }
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    for (std::size_t i = 0; i < iters; ++i) {
        do_not_optimize(fn());
    }
    auto t1 = Clock::now();

    double total_ns = std::chrono::duration<double, std::nano>(t1 - t0).count();

    BenchResult r;
    r.iters = iters;
    r.ns_per_op = total_ns / static_cast<double>(iters);
    r.ops_per_sec = 1e9 / r.ns_per_op;
    if (bytes > 0.0) {
        r.gbytes_per_sec = bytes / r.ns_per_op; // 1 byte/ns == 1 GB/s
    }
    return r;
}

// 打印一行基准结果
inline void report(const std::string& name, const BenchResult& r) {
    std::printf("%-28s %10.1f ns/op   %12.3f Mops/s",
                name.c_str(), r.ns_per_op, r.ops_per_sec / 1e6);
    if (r.gbytes_per_sec > 0.0) {
        std::printf("   %8.2f GB/s", r.gbytes_per_sec);
    }
    std::printf("\n");
}

} // namespace simd
