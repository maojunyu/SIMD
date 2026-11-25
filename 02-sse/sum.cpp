// Phase 2：SSE intrinsics 入门 —— 手写数组求和
// 关键点：
//   * __m128 一次放 4 个 float
//   * _mm_loadu_ps 未对齐加载（安全）；_mm_load_ps 要求 16 字节对齐（更快）
//   * _mm_add_ps 一条指令做 4 个加法
//
// 重要现象（本机实测，4MB 数组，见 benchmark 输出）：
//   * 标量 s += a[i] 约 10 GB/s  —— 瓶颈不是内存，是浮点加法的串行依赖链
//     （每个 addss 要等上一个的结果，约 4 个周期的延迟）。
//   * SSE 版本约 40 GB/s —— 4 个 lane 并行累加，把依赖链断成 4 条，正好 4 倍。
//   * 编译器默认【不会】自动向量化浮点归约，因为改变加法顺序会改变舍入结果；
//     加上 -ffast-math 后 GCC 才会用 AVX-512 向量化，可达 150+ GB/s。
//
// 第一课：先看反汇编（objdump），别凭感觉判断"标量慢/编译器会帮我向量化"。
#include <immintrin.h>
#include <cstddef>
#include <cstdio>
#include <vector>

#include <simd_common/benchmark.hpp>
#include <simd_common/print.hpp>

static float sum_scalar(const float* a, std::size_t n) {
    float s = 0.0f;
    for (std::size_t i = 0; i < n; ++i) s += a[i];
    return s;
}

static float sum_sse(const float* a, std::size_t n) {
    __m128 acc = _mm_setzero_ps(); // acc = [0,0,0,0]
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m128 v = _mm_loadu_ps(a + i);
        acc = _mm_add_ps(acc, v);
    }
    // 水平求和：把 acc 的 4 个 lane 加起来
    alignas(16) float tmp[4];
    _mm_store_ps(tmp, acc);
    float s = tmp[0] + tmp[1] + tmp[2] + tmp[3];
    // 尾部剩余元素（N 不是 4 的倍数时）
    for (; i < n; ++i) s += a[i];
    return s;
}

int main() {
    constexpr std::size_t N = 1u << 20;
    std::vector<float> a(N, 1.0f);

    float ref = sum_scalar(a.data(), N);
    float got = sum_sse(a.data(), N);
    std::printf("scalar = %f   sse = %f   (expect %f)\n",
                ref, got, static_cast<float>(N));

    // 看看寄存器里的数据长什么样
    simd::print_ps("first 4", _mm_loadu_ps(a.data()));

    double bytes = static_cast<double>(N) * 4.0;
    auto r1 = simd::benchmark([&] { return sum_scalar(a.data(), N); }, 10'000, 100, bytes);
    auto r2 = simd::benchmark([&] { return sum_sse(a.data(), N); }, 10'000, 100, bytes);
    simd::report("sum scalar", r1);
    simd::report("sum SSE", r2);
    return 0;
}
