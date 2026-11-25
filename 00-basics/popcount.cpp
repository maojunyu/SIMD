// Phase 0 练习：popcount（统计二进制中 1 的个数）
// 对比朴素实现 vs 编译器内置（编译成一条 popcnt 指令）
#include <cstdint>
#include <cstdio>

// 朴素实现：逐位统计
static int popcount_scalar(uint32_t x) {
    int c = 0;
    while (x) {
        c += x & 1u;
        x >>= 1;
    }
    return c;
}

// 编译器内置函数，在支持 POPCNT 的 CPU 上生成单条指令
static int popcount_builtin(uint32_t x) {
    return __builtin_popcount(x);
}

int main() {
    const uint32_t vals[] = {0u, 1u, 0xffffffffu, 0x0f0f0f0fu, 0xdeadbeefu};
    const int expected[] = {0, 1, 32, 16, 24};

    bool ok = true;
    for (int i = 0; i < 5; ++i) {
        int a = popcount_scalar(vals[i]);
        int b = popcount_builtin(vals[i]);
        std::printf("popcount(0x%08x) = %2d (scalar) / %2d (builtin)   expect %2d\n",
                    vals[i], a, b, expected[i]);
        if (a != expected[i] || b != expected[i]) ok = false;
    }
    std::printf(ok ? "PASS\n" : "FAIL\n");
    return ok ? 0 : 1;
}
