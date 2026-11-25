// Phase 1 实验：自动向量化
// 这个循环在 -O3 -march=native 下会被编译器自动向量化。
// 想看编译器做了什么，单独编译时加 -fopt-info-vec-optimized：
//   g++ -O3 -march=native -fopt-info-vec-optimized -c sum.cpp
#include <cstddef>
#include <cstdio>
#include <vector>

#include <simd_common/benchmark.hpp>

// 求和 kernel：无循环间依赖，最容易被自动向量化
static float sum(const float* a, std::size_t n) {
    float s = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        s += a[i];
    }
    return s;
}

int main() {
    constexpr std::size_t N = 1u << 20; // 1M floats = 4 MB
    std::vector<float> a(N, 1.0f);

    float r = sum(a.data(), N);
    std::printf("sum = %.1f (expect %.1f)\n", r, static_cast<float>(N));

    // 单次 op 读取 N*4 字节
    double bytes = static_cast<double>(N) * 4.0;
    auto res = simd::benchmark([&] { return sum(a.data(), N); },
                               10'000, 100, bytes);
    simd::report("sum (auto-vec?)", res);
    return (r == static_cast<float>(N)) ? 0 : 1;
}
