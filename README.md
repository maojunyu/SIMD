# SIMD 学习计划

> 目标：从零掌握 SIMD（单指令多数据）技术，能读懂、手写、调优向量化代码，并在实际项目里落地。
>
> 环境：AMD Ryzen 7 9800X3D (Zen 5, AVX-512) · WSL2 · GCC 15.2 / Clang

---

## 一、路线总览

| 阶段 | 主题 | 周期(约) | 产出 |
|------|------|----------|------|
| Phase 0 | 前置基础补课 | 1–2 天 | 位运算/内存对齐小练习 |
| Phase 1 | SIMD 概念 + 自动向量化 | 2–3 天 | 一个"编译器帮我向量化"的 demo |
| Phase 2 | Intrinsics 入门（SSE/128位） | 3–5 天 | 手写数组求和/点积/查表 |
| Phase 3 | AVX2（256位）+ FMA | 3–5 天 | 矩阵乘法、图像处理 kernel |
| Phase 4 | AVX-512（512位） | 2–3 天 | 掩码、压缩/扩展等高级用法 |
| Phase 5 | 实战项目 | 1–2 周 | 1 个完整可 benchmark 的项目 |
| Phase 6 | 性能分析与调优 | 持续 | 用 perf/反汇编定位瓶颈 |

> 每个阶段都「先跑通 → 再对比标量 → 再解释为什么快」三步走，别跳过性能对比。

---

## 快速开始（构建）

```bash
# 1. 配置（默认 Release + -march=native）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2. 编译（-j 并行）
cmake --build build -j

# 3. 运行某个示例
./build/02-sse/sse_sum

# 4. 跑全部自测（ctest）
ctest --test-dir build --output-on-failure
```

**常用选项：**
```bash
# 带调试符号（反汇编/调试更方便）
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo

# 关掉 -march=native，对比"无向量化"的慢版本
cmake -S . -B build -DSIMD_NATIVE=OFF
```

**已验证的一个关键现象**（本机 4MB 数组求和）：
| 版本 | 带宽 | 原因 |
|------|------|------|
| 标量 `s += a[i]` | ~10 GB/s | 浮点加法串行依赖链（延迟受限） |
| SSE 手写（4 lane） | ~40 GB/s | 4 条并行累加，正好 4 倍 |
| 编译器 + `-ffast-math` | ~157 GB/s | AVX-512 自动向量化 |

> 教训：浮点归约默认不会被自动向量化（改变加法顺序会改变舍入）。先反汇编，再下结论。

---

## 二、分阶段详解

### Phase 0 —— 前置基础
**要会的东西（不会就补）：**
- 二进制与位运算：`& | ^ ~ << >>`，字节序、十六进制读法
- C/C++ 基础：指针、`malloc`、栈上数组、函数指针
- 内存对齐：为什么 16/32/64 字节对齐对 SIMD 重要（`alignas`、`_mm_malloc`）
- 计算机组成常识：CPU 寄存器、流水线、缓存行（64B）、吞吐 vs 延迟

**练习：**
1. 手写一个 `popcount`（统计二进制中 1 的个数）的普通实现。
2. 写一个 16 字节对齐的结构体，用 `alignas(64)` 验证地址。

### Phase 1 —— 概念 + 自动向量化
**核心概念：**
- SISD vs SIMD：一条指令同时操作 N 个数据
- 数据并行 vs 任务并行：SIMD 属于数据并行
- 为什么快：指令条数下降 + 数据并行隐藏延迟

**关键实验（这一步很重要，建立直觉）：**
```bash
g++ -O2 -march=native -fopt-info-vec-optimized  main.cpp
```
用 `-fopt-info-vec` 看编译器到底把哪些循环自动向量化了。

**练习：**
- 写 5 个不同写法的数组求和循环，观察哪些能被 `-O3 -march=native` 自动向量化，哪些不能（比如带依赖的循环 vectorize 不了）。

### Phase 2 —— Intrinsics 入门（SSE，128 位）
**要点：**
- Intrinsics 是什么：把汇编指令封装成 C 函数，`#include <immintrin.h>`
- 数据类型：`__m128 / __m128i / __m128d`（float / int / double）
- 加载存储：`_mm_loadu_ps`（未对齐）/ `_mm_load_ps`（对齐，快）
- 算术：`_mm_add_ps`、`_mm_mul_ps`、`_mm_hadd_ps`（horizontal 慢，慎用）
- 混排与洗牌：`_mm_shuffle_ps`、`_mm_unpacklo_ps`
- 比较与掩码：`_mm_cmplt_ps` → 全 1/全 0 位
- **对齐与未对齐加载的代价差异**

**必做练习（从小到大）：**
1. 8 个 float 数组求和（`_mm_add_ps` 分两段）
2. 两个数组点积 `dot(a,b)`
3. 找数组最大值 `max`
4. `saxpy`：`y = a*x + y`（经典 BLAS kernel）
5. 数组查表/条件替换：`x[i] > 0 ? x[i] : -x[i]`（绝对值）

**查表资料：** Intel Intrinsics Guide（网页版，必收藏）

### Phase 3 —— AVX2 + FMA（256 位）
**要点：**
- 寄存器翻倍：`__m256`，一次处理 8 个 float / 4 个 double
- 上半/下半 128 位通道（lane）的坑：`_mm256_shuffle_ps` 只在 128 位内洗牌
- FMA：`_mm256_fmadd_ps` —— 乘加融合，一次算 `a*b+c`，比 `mul+add` 更快且更精确
- `_mm256_loadu_si256` / `_mm256_loadu2_m128` 等

**练习：**
1. 把 Phase 2 的所有练习升级到 AVX2，对比加速比
2. 矩阵乘法 `C = A*B`（先朴素，再加 blocking + FMA）
3. 图像灰度化 / 高斯模糊（把图像当 byte 数组处理）

### Phase 4 —— AVX-512（512 位）
**要点：**
- `__m512`，一次 16 个 float；ZMM 寄存器 + 掩码寄存器 `k0–k7`
- 掩码操作：`_mm512_mask_add_ps`、`_mm512_cmp_ps_mask`
- 压缩/扩展：`_mm512_maskz_compress_ps`（稀疏数据利器）
- `_mm512_fmadd_ps`、vpopcnt 等 Zen5 上很强的指令
- 注意：AVX-512 会导致降频（license-based frequency，不同 CPU 策略不同），实际项目要 benchmark 再决定

**练习：**
1. 用掩码实现"只对数组中满足条件的元素做加法"
2. 稀疏向量点积（用 compress）
3. 对比同一 kernel 在 AVX2 vs AVX-512 下的实际耗时（用 rdtsc 或 `std::chrono`）

### Phase 5 —— 实战项目（选 1 个做深）
- **矩阵乘法优化**：从 1 GFLOP/s 逐步优化到接近 CPU 理论峰值
- **图像处理库**：模糊、锐化、边缘检测、resize
- **JSON/文本解析加速**：`simdjson` 的简化版思路（字符分类、跳空白）
- **排序/去重**：SIMD 归并排序、SIMD 过滤
- **AI 推理算子**：写一个 SIMD 版的 `ReLU / Softmax / 量化`

### Phase 6 —— 性能分析与调优
**工具：**
- `perf stat` / `perf record`：看指令数、缓存 miss、分支预测
- 反汇编：`objdump -d` 或 Compiler Explorer 看生成的指令
- 基准测试框架：`google/benchmark` 或手写稳定计时
- `__rdtsc()` 或 `std::chrono::steady_clock` 测量单次 kernel

**要建立的思维：**
- 先正确，后优化；每次优化都要有 baseline 对比
- 关注「每周期指令数 / 内存带宽 / 缓存 miss」而不是只看耗时
- 数据在内存中的布局（SoA vs AoS）往往比指令本身更影响性能

---

## 三、常用命令速查

```bash
# 编译（针对本机 CPU 全优化 + 输出向量化报告）
g++ -O3 -march=native -fopt-info-vec-optimized -o main main.cpp

# 只跑 SSE2（禁用 AVX，观察差异）
g++ -O3 -msse2 -o main main.cpp

# 反汇编看生成的 SIMD 指令
objdump -d main | grep -E 'xmm|ymm|zmm' | head

# 查看 CPU 支持的指令集
grep -m1 -o 'avx[^ ]*\|sse[^ ]*\|fma' /proc/cpuinfo | sort -u

# 性能统计
perf stat ./main
```

---

## 四、目录结构（边学边填）

```
SIMD/
├── README.md            # 本计划
├── 00-basics/           # 位运算、对齐、popcount
├── 01-autovec/          # 自动向量化实验
├── 02-sse/              # SSE intrinsics 练习
├── 03-avx2/             # AVX2 + FMA 练习
├── 04-avx512/           # AVX-512 练习
├── 05-project/          # 实战项目
├── common/              # 计时器、benchmark 工具、打印工具
└── notes/               # 学习笔记
```

---

## 五、推荐资源

- **Intel Intrinsics Guide**（网页，必收藏）：查每个 intrinsic 的语义、延迟、吞吐
- **Compiler Explorer**（godbolt.org）：实时看 C++ → 汇编，理解向量化
- 书籍：
  - 《Computer Systems: A Programmer's Perspective》(CSAPP) 第 5 章
  - 《Hacker's Delight》（位运算脑洞）
  - Agner Fog《Optimizing software in C++》+ 指令延迟表
- 开源代码精读：`simdjson`、`highway`、`xsimd`、`Eigen`、`oneDNN`
- 视频：Performance-Aware Programming（Casey Muratori）里的 SIMD 章节

---

## 六、检验标准（学完应能）

- [ ] 能看懂 `objdump` 输出的 XMM/YMM/ZMM 指令
- [ ] 手写矩阵乘法比朴素版快 5–20 倍
- [ ] 知道什么循环能自动向量化、什么不能
- [ ] 会权衡 AVX2 vs AVX-512 的降频 trade-off
- [ ] 能说出 SoA vs AoS、对齐加载 vs 未对齐加载的性能差异
- [ ] 能独立用 perf 定位一个性能瓶颈

> 建议：从 Phase 1 的自动向量化实验开始，先"看见"向量化，再进入手写。遇到问题先反汇编，再看 Intrinsics Guide。
