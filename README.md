# Parallel Letter Frequency: High-Performance C++ Implementation

A state-of-the-art, highly optimized C++ implementation for counting letter frequencies in text views in parallel. This project demonstrates extreme optimization techniques at both the software concurrency level and the hardware CPU/microarchitecture level, with a primary focus on Apple Silicon (macOS) and modern multicore processors.

---

## 1. Architectural Approach

The solution implements a **high-throughput, memory-resident Map-Reduce** model tailored to modern CPU cache and execution pipeline architectures:

### Concurrency & Dynamic Load Balancing
*   **Asymmetric Core Optimization:** Apple Silicon utilizes heterogeneous architectures with Performance (P) and Efficiency (E) cores. Static workload partitioning causes slower E-cores to bottleneck the overall execution (straggler effect). 
*   **Dynamic Work-Stealing/Granular Scheduling:** Instead of static chunking, worker threads dynamically query a global atomic index (`std::atomic<size_t>`) to acquire work in small batches (e.g., 16 strings). This balances the scheduler's atomic overhead with the hardware's heterogeneous processing speed, maximizing multi-core throughput.

### Memory & Cache Line Optimization
*   **False Sharing Elimination:** In multi-threaded environments, threads writing to adjacent memory addresses residing on the same cache line force CPU cache-coherency cycles (invalidating lines across L1/L2 caches). The local thread accumulators (`frequency_map`) are aligned to the CPU's specific L1 cache line size using `alignas(VT_CACHE_LINE)` (128 bytes on Apple Silicon, 64 bytes on x86_64).
*   **Memory Bandwidth Utilization:** The main loop avoids unnecessary heap allocation or locking. Threads process strings entirely in registers or L1 cache, aggregating results into thread-local maps, and perform a single lock-free reduction at the end.

### Microarchitectural Instruction Pipelining
*   **Branchless Lookup Table (LUT):** Character classification (`[A-Za-z]`) and folding are performed branchlessly in $O(1)$ time. This prevents CPU pipeline stalls caused by branch mispredictions.
*   **4-Way Loop Unrolling:** The fallback scalar loop is unrolled four times to expose instruction-level parallelism (ILP) to the CPU out-of-order execution engine, saturating the multiple arithmetic logic units (ALUs).

### Hardware-Accelerated Vectorization (Apple Silicon)
*   **Apple Accelerate vImage Integration:** On macOS, the implementation utilizes the native `vImageHistogramCalculation_Planar8` function. This leverages the hardware's vector units (NEON and Apple's proprietary AMX coprocessor) to count all 256 byte values directly at/near memory bandwidth (~15–30 GB/s), bypassing scalar loop instructions entirely.

---

## 2. Evolution of Versions

The codebase contains several historical iterations, allowing performance comparison:

1.  **`exercism` / `v1.x.x` (Baseline):** Standard C++ map-reduce using associative containers (`std::map`), which suffers from node allocation overheads, pointer chasing, and heavy locking/synchronization.
2.  **`v2.0.0` & `v2.1.1`:** Shift to contiguous array-based frequency tables (`std::array<size_t, 32>`) and branchless ASCII LUTs. Introduces parallel algorithms (`std::transform_reduce` with execution policies).
3.  **`v3.0.0` (`latest` root baseline):** Introduces cache-line alignment to prevent false sharing and 4-way scalar loop unrolling to maximize pipeline occupancy.
4.  **`v3.1.0_M1` (Apple Silicon Optimized):** Integrates the Apple Accelerate framework for hardware-accelerated histogram calculation and uses atomic-based dynamic load-balancing for asymmetric cores.

---

## 3. How to Build

The project uses CMake as its build system. A modern compiler supporting C++23 is recommended.

### Prerequisites

*   **macOS:** The Accelerate framework is included natively with macOS. If you wish to build with TBB (Threading Building Blocks) for standard parallel execution policies, install it via Homebrew:
    ```bash
    brew install tbb
    ```
*   **Linux:** Install the Threading Building Blocks library:
    ```bash
    sudo apt-get install libtbb-dev
    ```

### Compilation

Create a build directory, configure CMake, and compile the targets:

```bash
# Configure with tests and benchmarks enabled
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DEXERCISM_RUN_ALL_TESTS=ON -DEXERCISM_INCLUDE_BENCHMARK=ON

# Compile the project
cmake --build build
```

---

## 4. How to Run

### Running Unit Tests
To run the Catch2-based test suite and verify implementation correctness:
```bash
./build/parallel-letter-frequency
```

### Running Benchmarks
To run the microbenchmarks comparing the latest implementation against historical baselines:
```bash
# Navigate to the benchmark folder and compile
cmake -B bench/build -S bench/ -DCMAKE_BUILD_TYPE=Release
cmake --build bench/build

# Execute Google Benchmark
./bench/build/benchmark_parallel_letter_frequency
```

---

## 5. Performance Explainer

For a deep dive into the microarchitectural analysis of C++20 coroutines, dynamic allocator bottlenecks, register-spilling, and detailed benchmark plots, refer to the [explainer.md](file:///Users/viraltaco_/Exercism/cpp/parallel-letter-frequency/explainer.md) document in the workspace root.