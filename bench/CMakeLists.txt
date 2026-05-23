cmake_minimum_required(VERSION 3.10)
project(LetterFrequencyBenchmarks)

set(CMAKE_CXX_STANDARD 23)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)

include(FetchContent)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

add_executable(benchmark_parallel_letter_frequency
    benchmark.cpp
    includes/exercism/parallel_letter_frequency.cpp # exercism
    includes/v1_1_1/parallel_letter_frequency.cpp   # v1.1.1
    includes/v1_1_5/parallel_letter_frequency.cpp   # v1.1.5
    includes/v1_1_6/parallel_letter_frequency.cpp   # v1.1.6
    includes/v1_2_4/parallel_letter_frequency.cpp   # v1.2.4
    includes/v2_0_0/parallel_letter_frequency.cpp   # v2.0.0
    includes/v2_1_1/parallel_letter_frequency.cpp   # v2.1.1
    includes/v3_1_0~M1/parallel_letter_frequency.cpp # v3_1_0~M1
    ../parallel_letter_frequency.cpp                # latest
)
target_compile_options(benchmark_parallel_letter_frequency PRIVATE -DNDEBUG -Wall -Wextra -O3 -march=native)

target_link_libraries(benchmark_parallel_letter_frequency PRIVATE benchmark::benchmark benchmark::benchmark_main)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "(GNU|Clang)")
    find_package(TBB)
    if (TBB_FOUND)
        target_link_libraries(benchmark_parallel_letter_frequency PRIVATE TBB::tbb)
    endif()
endif()

if(APPLE)
    target_link_libraries(benchmark_parallel_letter_frequency PRIVATE "-framework Accelerate")
endif()
