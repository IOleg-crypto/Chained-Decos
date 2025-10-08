cmake_minimum_required(VERSION 3.16)

# Simple performance benchmarks executable
add_executable(benchmarks PerformanceBenchmark.cpp)

# Include directories
target_include_directories(benchmarks PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Engine
    ${CMAKE_SOURCE_DIR}/src/Game
)

# Link against project libraries
target_link_libraries(benchmarks
    ChainedDecosEngine
    ChainedDecosGame
)

# Set C++ standard
target_compile_features(benchmarks PRIVATE cxx_std_20)

# Enable optimizations for benchmarks
target_compile_options(benchmarks PRIVATE
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
    $<$<CONFIG:Debug>:-O2 -g>
)