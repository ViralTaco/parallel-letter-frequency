#pragma once /// Copyright 2026 viraltaco_ <https://viraltaco.com>
#ifndef vt_parallel_letter_frequency_m1
#define vt_parallel_letter_frequency_m1 "com.viraltaco.letter-frequency v" "3.1.0~M1"

#include <string_view> // std::string_view
#include <algorithm>   // std::ranges::all_of
#include <numeric>     // std::transform_reduce
#include <ranges>      // std::views::take
#include <vector>      // std::vector
#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <future>      // std::async, std::future
#include <thread>      // std::thread::hardware_concurrency
#include <atomic>      // std::atomic

#if defined(__ARM_NEON) || defined(__aarch64__)
  #include <arm_neon.h>
  #define VT_M1_USE_NEON 1
#else
  #define VT_M1_USE_NEON 0
#endif

#if defined(__APPLE__)
  #include <Accelerate/Accelerate.h>
  #define VT_USE_ACCELERATE 1
#else
  #define VT_USE_ACCELERATE 0
#endif

// MARK: Cache Line Size Detection (Prevents False Sharing in multithreading)
#if defined(__APPLE__) && defined(__aarch64__)
  #define VT_CACHE_LINE 128z
#else
  #define VT_CACHE_LINE 64z
#endif

// Compiler-Specific Vectorization Hints
#if defined(__clang__)
  #define VT_VECTORIZE_LOOP _Pragma("clang loop vectorize(enable)")
#elif defined(__GNUC__)
  #define VT_VECTORIZE_LOOP _Pragma("GCC ivdep")
#else
  #define VT_VECTORIZE_LOOP
#endif

namespace parallel_letter_frequency::inline v3_1_0_M1 {

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
    static constexpr auto kTableSize = 32z;
    using self_type = typename std::array<count_type, kTableSize>;

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
      const auto len = static_cast<ssize_type>(str.size());
      if (len == 0) return;

#if VT_USE_ACCELERATE
      vImage_Buffer srcBuffer = {
        .data = const_cast<void*>(static_cast<const void*>(str.data())),
        .height = 1,
        .width = static_cast<vImagePixelCount>(len),
        .rowBytes = static_cast<size_t>(len)
      };

      vImagePixelCount local_hist[256] = {0};
      vImage_Error err = vImageHistogramCalculation_Planar8(&srcBuffer, local_hist, kvImageNoFlags);
      if (err == kvImageNoError) {
        for (char c = 'a'; c <= 'z'; ++c) {
          self_[c - 'a'] += local_hist[static_cast<std::uint8_t>(c)] + local_hist[static_cast<std::uint8_t>(c - 32)];
        }
        return;
      }
#endif

      auto const* beg = reinterpret_cast<const std::uint8_t*>(str.data());
      alignas(VT_CACHE_LINE) self_type a0{}, a1{}, a2{}, a3{};
      auto i = 0z;

#if VT_M1_USE_NEON
      if (len >= 32) {
        uint8x16_t v_A = vdupq_n_u8('A');
        uint8x16_t v_Z = vdupq_n_u8('Z');
        uint8x16_t v_DF = vdupq_n_u8(0xDF);
        uint8x16_t v_31 = vdupq_n_u8(31);

        for (; (i + 31) < len; i += 32) {
          uint8x16_t v0 = vld1q_u8(beg + i);
          uint8x16_t v1 = vld1q_u8(beg + i + 16);

          uint8x16_t upper0 = vandq_u8(v0, v_DF);
          uint8x16_t upper1 = vandq_u8(v1, v_DF);

          uint8x16_t is_let0 = vandq_u8(vcgeq_u8(upper0, v_A), vcleq_u8(upper0, v_Z));
          uint8x16_t is_let1 = vandq_u8(vcgeq_u8(upper1, v_A), vcleq_u8(upper1, v_Z));

          uint8x16_t idx0 = vsubq_u8(upper0, v_A);
          uint8x16_t idx1 = vsubq_u8(upper1, v_A);

          uint8x16_t res0 = vbslq_u8(is_let0, idx0, v_31);
          uint8x16_t res1 = vbslq_u8(is_let1, idx1, v_31);

          alignas(16) std::uint8_t buf[32];
          vst1q_u8(buf, res0);
          vst1q_u8(buf + 16, res1);

          // 4 independent accumulators to utilize M1 superscalar execution ports
          ++a0[buf[0]];  ++a1[buf[1]];  ++a2[buf[2]];  ++a3[buf[3]];
          ++a0[buf[4]];  ++a1[buf[5]];  ++a2[buf[6]];  ++a3[buf[7]];
          ++a0[buf[8]];  ++a1[buf[9]];  ++a2[buf[10]]; ++a3[buf[11]];
          ++a0[buf[12]]; ++a1[buf[13]]; ++a2[buf[14]]; ++a3[buf[15]];
          ++a0[buf[16]]; ++a1[buf[17]]; ++a2[buf[18]]; ++a3[buf[19]];
          ++a0[buf[20]]; ++a1[buf[21]]; ++a2[buf[22]]; ++a3[buf[23]];
          ++a0[buf[24]]; ++a1[buf[25]]; ++a2[buf[26]]; ++a3[buf[27]];
          ++a0[buf[28]]; ++a1[buf[29]]; ++a2[buf[30]]; ++a3[buf[31]];
        }
      }
#endif

      for (; i < len; ++i) {
        ++a0[index_of(beg[i])];
      }

      VT_VECTORIZE_LOOP
      for (auto j = 0z; j < kTableSize; ++j) {
        self_[j] += a0[j] + a1[j] + a2[j] + a3[j];
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

    auto const kLen = rec.size();
    auto const hw_threads = std::thread::hardware_concurrency();
    auto const num_workers = hw_threads > 0 ? hw_threads : 8;

    // Dynamic load-balancing to distribute load optimally across asymmetric P/E cores
    std::atomic<size_type> global_index{0};
    auto futures = std::vector<std::future<frequency_map>>{};
    futures.reserve(num_workers);

    for (int t = 0; t < static_cast<int>(num_workers); ++t) {
      futures.push_back(std::async(std::launch::async, [&rec, &global_index, kLen]() {
        auto local_map = frequency_map{};
        // Batch size of 16 balances atomic operation overhead with dynamic granularity
        constexpr size_type kBatchSize = 16;
        while (true) {
          auto start_idx = global_index.fetch_add(kBatchSize, std::memory_order_relaxed);
          if (start_idx >= kLen) break;
          auto end_idx = std::min(start_idx + kBatchSize, kLen);
          for (auto i = start_idx; i < end_idx; ++i) {
            local_map.insert(rec[i]);
          }
        }
        return local_map;
      }));
    }

    for (auto& f : futures) {
      self_ += f.get();
    }
  }

  [[nodiscard]] auto empty() const noexcept -> bool { return self_.empty(); }
  [[nodiscard]] auto operator[](const key_type k) const noexcept -> count_type { return self_[k]; }
};

} // namespace parallel_letter_frequency::inline v3_1_0_M1

#endif // vt_parallel_letter_frequency_m1
