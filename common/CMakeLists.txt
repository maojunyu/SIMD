# 公共工具库：header-only（计时器、benchmark、SIMD 向量打印）
add_library(simd_common INTERFACE)
target_include_directories(simd_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(simd_common INTERFACE cxx_std_17)
