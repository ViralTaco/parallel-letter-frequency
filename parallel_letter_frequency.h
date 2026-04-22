#pragma once /// Copyright 2026 viraltaco_ <https://viraltaco.com>
#ifndef vt_parallel_letter_frequency
#define vt_parallel_letter_frequency "com.viraltaco.letter-frequency v" "2.1.0"

#include <numeric>     // std::transform_reduce
#include <string_view> // std::string_view
#include <algorithm>   // std::ranges::all_of
#include <ranges>      // std::views::take
#include <array>       // std::array
#include <vector>      // std::vector

#ifndef __APPLE__
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ
#endif

namespace parallel_letter_frequency::inline v2_1_0 {
class frequency {
public:
  using string_view = typename std::string_view;
  using record_type = typename std::vector<string_view>;
  using key_type    = typename string_view::value_type;
  using size_type   = decltype (0zu);
  using ssize_type  = decltype (0z);
  using count_type  = size_type;

  class frequency_map {
  private:
    // MARK: Invariants
    static constexpr auto kTableSize = 32z;
    using self_type  = typename std::array<count_type, kTableSize>;

    // MARK: Members
    self_type self = self_type{};

  protected:
    // MARK Text Utils
    static constexpr auto upper = [](auto c) noexcept { return 'A' <= c and c <= 'Z'; };
    static constexpr auto lower = [](auto c) noexcept { return 'a' <= c and c <= 'z'; };
    static constexpr auto alpha = [](auto c) noexcept { return lower(c) or upper(c); };
    static constexpr auto kIndexOf = [] {
      auto arr = std::array<char, 256>{};
      arr.fill(kTableSize - 1);
      for (auto c = 'A'; c <= 'Z'; ++c) arr[c] = c - 'A';
      for (auto c = 'a'; c <= 'z'; ++c) arr[c] = c - 'a';
      return arr;
    }();
    /// P: c is an ascii letter.
    /// R: The lexographic index (starting from 0 for 'a', ignoring case).
    [[nodiscard]] static constexpr auto index_of(const auto k) noexcept -> size_type {
      return static_cast<size_type> (kIndexOf[k]);
    }

  public:
    // MARK: Rule of Five
    frequency_map() noexcept = default;

    explicit frequency_map(const string_view str) noexcept { insert(str); }

    frequency_map(frequency_map &&map) noexcept = default;
    frequency_map& operator =(frequency_map &&map) noexcept = default;

    frequency_map(frequency_map const &map) noexcept = default;
    frequency_map& operator =(frequency_map const &map) noexcept = default;

    // MARK: methods
    [[nodiscard]] auto empty() const noexcept -> bool {
      return std::ranges::all_of(self | std::views::take(26), [] (auto c) { return c == 0; });
    }

    auto insert(const string_view str) noexcept -> void {
      auto const* beg = str.data();
      const auto kLen = static_cast<ssize_type> (str.size());
      
      if (kLen < 64) {
        for (auto i = 0z; i != kLen; ++i) {
          ++self[index_of(beg[i])];
        }
      } else {
        // 4 accumulators
        self_type a0{}, a1{}, a2{}, a3{};
        auto i = 0z;
        
        for (; (i + 3) < kLen; i += 4) {
          ++a0[index_of(beg[i + 0])];
          ++a1[index_of(beg[i + 1])];
          ++a2[index_of(beg[i + 2])];
          ++a3[index_of(beg[i + 3])];
        }
        
        // remainder of kLen after mod 4
        for (; i < kLen; ++i) {
          ++a0[index_of(beg[i])];
        }
        
        for (auto j = 0z; j < kTableSize; ++j) {
          self[j] += a0[j] + a1[j] + a2[j] + a3[j];
        }
      }
    }

    auto operator +=(frequency_map const& other) noexcept -> frequency_map& {
      for (auto i = 0; i < kTableSize; ++i) {
        self[i] += other.self[i];
      }
      return *this;
    }
      
    /// P: k is a valid key.
    /// R: the frequency value for the key.
    [[nodiscard]] auto operator [](const key_type k) const -> count_type {
      const auto kIdx = index_of(k);
      return static_cast<bool> (kIdx < 26) * self[kIdx];
    }
  };

private:
  frequency_map self;

public:
  explicit frequency(record_type const& rec)
    : self(
      std::transform_reduce(PAR_UNSEQ
        rec.cbegin()
      , rec.cend()
      , frequency_map{}
      , [] (frequency_map a, frequency_map const& b) { return a += b; }
      , [] (const string_view str) { return frequency_map{str}; }) // Transform
    )
  {}

  [[nodiscard]] auto empty() const noexcept -> bool { return self.empty(); }
  [[nodiscard]] auto operator [](const key_type k) const { return self[k]; }
};
}  // namespace parallel_letter_frequency::inline v2_1_0

namespace parallel_letter_frequency {
  namespace latest = v2_1_0;
} // namespace parallel_letter_frequency
#endif  // ndef vt_parallel_letter_frequency
