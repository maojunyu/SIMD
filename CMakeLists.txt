cmake_minimum_required(VERSION 3.20)

project(simd_learning
    VERSION 0.1.0
    DESCRIPTION "SIMD learning playground (SSE / AVX2 / AVX-512)"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# 全局设置
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认用 Release（带 -O3 -DNDEBUG），调试时可用 -DCMAKE_BUILD_TYPE=RelWithDebInfo
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 Release Debug RelWithDebInfo MinSizeRel)
endif()

# ---------------------------------------------------------------------------
# 选项
# ---------------------------------------------------------------------------
# 默认 -march=native 针对本机 CPU 生成指令；关掉可观察"无向量化"的慢版本
option(SIMD_NATIVE "Compile with -march=native for this CPU" ON)

# ---------------------------------------------------------------------------
# 编译选项
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive- /arch:AVX2)
else()
    add_compile_options(-Wall -Wextra)
    if(SIMD_NATIVE)
        add_compile_options(-march=native)
    endif()
endif()

# ---------------------------------------------------------------------------
# 测试
# ---------------------------------------------------------------------------
enable_testing()

# ---------------------------------------------------------------------------
# 辅助函数：添加一个示例可执行文件，并自动链接公共工具库
# 用法：simd_add_example(<target> <source...>)
# ---------------------------------------------------------------------------
function(simd_add_example target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE simd_common)
endfunction()

# ---------------------------------------------------------------------------
# 子目录（common 必须最先，供其它目标链接）
# ---------------------------------------------------------------------------
add_subdirectory(common)

add_subdirectory(00-basics)
add_subdirectory(01-autovec)
add_subdirectory(02-sse)
add_subdirectory(03-avx2)
add_subdirectory(04-avx512)
add_subdirectory(05-project)
