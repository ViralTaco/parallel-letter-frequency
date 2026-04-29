#pragma once /// Copyright 2026 viraltaco_ <https://viraltaco.com>
#ifndef vt_parallel_letter_frequency
#define vt_parallel_letter_frequency "com.viraltaco.letter-frequency v3.0.0"

#include <string_view> // std::string_view
#include <algorithm>   // std::ranges::all_of
#include <numeric>     // std::transform_reduce
#include <ranges>      // std::views::take
#include <vector>      // std::vector
#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <future>      // std::async, std::future
#include <thread>      // std::thread::hardware_concurrency

// MARK: Architecture & Compiler Detection Macros

// 1. Cache Line Size Detection (Prevents False Sharing in multithreading)
#if defined(__APPLE__) && defined(__aarch64__)
  // Apple Silicon (M1/M2/M3) uses 128-byte L1 cache lines
  #define VT_CACHE_LINE 128z
#else
  // x86_64 (Intel/AMD) and most others use 64-byte L1 cache lines
  #define VT_CACHE_LINE 64z
#endif

// 2. Parallel Execution Policy Detection
// Apple Clang historically lacks <execution> support out of the box.
#if defined(__cpp_lib_execution) && !defined(__APPLE__)
  #include <execution>
  #define VT_USE_STD_EXECUTION 1
#else
  #define VT_USE_STD_EXECUTION 0
#endif

// 3. Compiler-Specific Vectorization Hints
#if defined(__clang__)
  #define VT_VECTORIZE_LOOP _Pragma("clang loop vectorize(enable)")
#elif defined(__GNUC__)
  #define VT_VECTORIZE_LOOP _Pragma("GCC ivdep")
#else
  #define VT_VECTORIZE_LOOP
#endif

namespace parallel_letter_frequency::inline v3_0_0 {

class frequency {
public:
  using string_view = typename std::string_view;
  using record_type = typename std::vector<string_view>;
  using key_type    = typename string_view::value_type;
  using size_type   = decltype(0zu);
  using ssize_type  = decltype(0z);
  using count_type  = size_type;

  class frequency_map {
  private:
    // 32 elements = 256 bytes. 
    // Maps perfectly to 1x AVX-256 register (x64) or 16x 128-bit NEON registers (ARM64).
    static constexpr auto kTableSize = 32z;
    using self_type = typename std::array<count_type, kTableSize>;

    // Aligned to the architecture's specific L1 cache line size
    alignas(VT_CACHE_LINE) self_type self_ = {};

  protected:
    // Branchless LUT: Maps [A-Za-z] to 0-25. Discard bin maps to 31.
    static constexpr auto kIndexOf = [] {
      auto arr = std::array<std::uint8_t, 256>{};
      arr.fill(kTableSize - 1z); 
      for (auto c = 'A'; c <= 'Z'; ++c) arr[c] = c - 'A';
      for (auto c = 'a'; c <= 'z'; ++c) arr[c] = c - 'a';
      return arr;
    }();

    [[nodiscard]] static constexpr auto index_of(const auto k) noexcept -> size_type {
      return static_cast<size_type>(kIndexOf[static_cast<std::uint8_t>(k)]);
    }

  public:
    frequency_map() noexcept = default;
    explicit frequency_map(const string_view str) noexcept { insert(str); }

    frequency_map(frequency_map&&) noexcept = default;
    auto operator=(frequency_map&&) noexcept -> frequency_map& = default;

    frequency_map(frequency_map const&) noexcept = default;
    auto operator=(frequency_map const&) noexcept -> frequency_map& = default;

    [[nodiscard]] auto empty() const noexcept -> bool {
      return std::ranges::all_of(self_ | std::views::take(26), [](auto c) { return !c; });
    }

    auto insert(const string_view str) noexcept -> void {
      auto const* beg = str.data();
      const auto len = static_cast<ssize_type>(str.size());
      
      if (len < 64) {
        for (auto i = 0z; i != len; ++i) {
          ++self_[index_of(beg[i])];
        }
      } else {
        // 4-way Unroll to saturate superscalar decode & execution units (M1 & x64)
        alignas(VT_CACHE_LINE) self_type a0{}, a1{}, a2{}, a3{};
        auto i = 0z;
      
        for (; (i + 3) < len; i += 4) {
          ++a0[index_of(beg[i | 0])];
          ++a1[index_of(beg[i | 1])];
          ++a2[index_of(beg[i | 2])];
          ++a3[index_of(beg[i | 3])];
        }
        
        for (; i < len; ++i) {
          ++a0[index_of(beg[i])];
        }
        
        VT_VECTORIZE_LOOP
        for (auto j = 0z; j < kTableSize; ++j) {
          self_[j] += a0[j] + a1[j] + a2[j] + a3[j];
        }
      }
    }

    auto operator+=(frequency_map const& other) noexcept -> frequency_map& {
      VT_VECTORIZE_LOOP
      for (auto i = 0z; i < kTableSize; ++i) {
        self_[i] += other.self_[i];
      }
      return *this;
    }
      
    [[nodiscard]] auto operator[](const key_type k) const noexcept -> count_type {
      const auto idx = index_of(k);
      return static_cast<count_type>(idx < 26) * self_[idx];
    }
  };

private:
  frequency_map self_;

public:
  explicit frequency(record_type const& rec) {
    if (rec.empty()) return;

#if VT_USE_STD_EXECUTION
    // x86_64 / Linux / Windows: Use standard parallel execution policies
    self_ = std::transform_reduce(
      std::execution::par_unseq,
      rec.cbegin(), rec.cend(),
      frequency_map{},
      [](frequency_map a, frequency_map const& b) { return a += b; },
      [](const string_view str) { return frequency_map{str}; }
    );
#else
    // Apple Silicon / Fallback: Native std::async chunking Map-Reduce
    auto const kLen = rec.size();
    auto const hw_threads = std::thread::hardware_concurrency();
    auto const num_workers = hw_threads > 0 ? hw_threads : 8;
    auto const chunk_size = (kLen + num_workers - 1) / num_workers;

    auto futures = std::vector<std::future<frequency_map>>{};
    futures.reserve(num_workers + 0zu);

    for (size_type t = 0; t < num_workers; ++t) {
      auto const start_idx = t * chunk_size;
      if (start_idx >= kLen) break;
      
      auto const end_idx = std::min(start_idx + chunk_size, kLen);

      futures.push_back(std::async(std::launch::async, [&rec, start_idx, end_idx]() {
        auto local_map = frequency_map{};
        for (auto i = start_idx; i < end_idx; ++i) {
          local_map += frequency_map{rec[i]};
        }
        return local_map;
      }));
    }

    for (auto& f : futures) {
      self_ += f.get();
    }
#endif
  }

  [[nodiscard]] auto empty() const noexcept -> bool { return self_.empty(); }
  [[nodiscard]] auto operator[](const key_type k) const noexcept -> count_type { return self_[k]; }
};

} // namespace parallel_letter_frequency::inline v3_0_0

namespace parallel_letter_frequency {
  namespace latest = v3_0_0;
}

#endif // vt_parallel_letter_frequency
