#pragma once
#include <immintrin.h>
#include <cstdio>

// 打印 SIMD 向量（float / int），方便调试时"看见"寄存器里的数据
namespace simd {

inline void print_ps(const char* name, __m128 v) {
    alignas(16) float a[4];
    _mm_store_ps(a, v);
    std::printf("%-14s = [ %9.3f %9.3f %9.3f %9.3f ]\n",
                name, a[0], a[1], a[2], a[3]);
}

inline void print_ps(const char* name, __m256 v) {
    alignas(32) float a[8];
    _mm256_store_ps(a, v);
    std::printf("%-14s = [ %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f ]\n",
                name, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}

inline void print_ps(const char* name, __m512 v) {
    alignas(64) float a[16];
    _mm512_store_ps(a, v);
    std::printf("%-14s = [ %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f ... ]\n",
                name, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}

inline void print_epi32(const char* name, __m128i v) {
    alignas(16) int a[4];
    _mm_store_si128(reinterpret_cast<__m128i*>(a), v);
    std::printf("%-14s = [ %d %d %d %d ]\n", name, a[0], a[1], a[2], a[3]);
}

inline void print_epi32(const char* name, __m256i v) {
    alignas(32) int a[8];
    _mm256_store_si256(reinterpret_cast<__m256i*>(a), v);
    std::printf("%-14s = [ %d %d %d %d %d %d %d %d ]\n",
                name, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}

} // namespace simd
