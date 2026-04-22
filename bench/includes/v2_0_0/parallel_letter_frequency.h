#pragma once /// Copyright 2026 viraltaco_ <https://viraltaco.com/parallel-letter-frequency>
#ifndef vt_parallel_letter_frequency_v2_0_0
#define vt_parallel_letter_frequency_v2_0_0 "com.viraltaco.letter-frequency v2.0.0"

#include <numeric>     // std::transform_reduce
#include <string_view> // std::string_view
#include <array>       // std::array
#include <vector>      // std::vector
#include <cstdint>     // std::uint8_t
#include <algorithm>   // std::ranges::all_of
#include <ranges>      // std::views::take

// Use standard feature testing macro for execution policies instead of OS macros
#if defined(__cpp_lib_execution)
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ
#endif

namespace parallel_letter_frequency {
inline namespace v2_0_0 {

class frequency {
public:
  using string_view = std::string_view;
  using record_type = std::vector<string_view>;
  using key_type    = string_view::value_type;
  using size_type   = std::size_t;
  using count_type  = size_type;

  class freq_map {
  private:
    // Padded to 32 (power of 2) for clean AVX2/AVX-512 auto-vectorization.
    // Cache-line aligned (64 bytes) to prevent false sharing in parallel reductions.
    alignas(64) std::array<count_type, 32> counts_{};

    // Branchless lookup table. Maps [A-Za-z] to 0-25. 
    // All other characters map to 31 (our safe "discard" bin).
    static constexpr auto kLut = []() {
      auto t = std::array<std::uint8_t, 256>{};
      t.fill(31); 
      for (std::uint8_t c = 'a'; c <= 'z'; ++c) t[c] = c - 'a';
      for (std::uint8_t c = 'A'; c <= 'Z'; ++c) t[c] = c - 'A';
      return t;
    }();

  public:
    freq_map() noexcept = default;

    explicit freq_map(const string_view str) noexcept { insert(str); }

    freq_map(freq_map&&) noexcept = default;
    auto operator=(freq_map&&) noexcept -> freq_map& = default;

    freq_map(const freq_map&) noexcept = default;
    auto operator=(const freq_map&) noexcept -> freq_map& = default;

    auto insert(const string_view str) noexcept -> void {
      auto const len = str.size();
      auto const* ptr = reinterpret_cast<std::uint8_t const*>(str.data());

      // Fast-path: Avoids the initialization overhead of the 4 accumulators on tiny strings
      if (len < 64) {
        for (size_type i = 0; i < len; ++i) {
          counts_[kLut[ptr[i]]]++;
        }
        return;
      }

      // 4-way unrolled accumulators for maximal Instruction-Level Parallelism (ILP).
      // This eliminates latency stalls caused by consecutive identical characters.
      alignas(64) std::array<count_type, 32> c0{}, c1{}, c2{}, c3{};
      size_type i = 0;
      
      for (; i + 3 < len; i += 4) {
        c0[kLut[ptr[i + 0]]]++;
        c1[kLut[ptr[i + 1]]]++;
        c2[kLut[ptr[i + 2]]]++;
        c3[kLut[ptr[i + 3]]]++;
      }
      
      // Epilogue for remaining elements
      for (; i < len; ++i) {
        c0[kLut[ptr[i]]]++;
      }

      // Fold local accumulators back into main counts.
      // Iterates unconditionally through 32 elements to let the compiler vectorize the fold.
      for (size_type k = 0; k < 32; ++k) {
        counts_[k] += c0[k] + c1[k] + c2[k] + c3[k];
      }
    }

    auto operator+=(const freq_map& other) noexcept -> freq_map& {
      // Loop over exactly 32 limits branching and translates to 1 AVX-256 add instruction.
      for (size_type i = 0; i < 32; ++i) {
        counts_[i] += other.counts_[i];
      }
      return *this;
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
      // Look only at valid alphabetical counts to determine emptiness.
      return std::ranges::all_of(counts_ | std::views::take(26), 
                                 [](auto v) { return v == 0; });
    }

    [[nodiscard]] auto operator[](const key_type k) const noexcept -> count_type {
      auto const idx = kLut[static_cast<std::uint8_t>(k)];
      return idx < 26 ? counts_[idx] : 0;
    }
  };

private:
  freq_map map_;

public:
  explicit frequency(const record_type& rec)
    : map_(std::transform_reduce(
        PAR_UNSEQ                                                  // Thread execution policy
        rec.cbegin(), rec.cend(),                                  // Source span
        freq_map{},                                                // Initial/Dest object
        [](freq_map a, const freq_map& b) { return a += b; },      // Reduction 
        [](const string_view str) { return freq_map{str}; }        // Transformation
      )) 
  {}

  [[nodiscard]] auto empty() const noexcept -> bool { return map_.empty(); }
  [[nodiscard]] auto operator[](const key_type k) const noexcept -> count_type { return map_[k]; }
};

} // inline namespace v2_0_0
} // namespace parallel_letter_frequency

#endif // vt_parallel_letter_frequency