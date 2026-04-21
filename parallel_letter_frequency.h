#pragma once /// Copyright 2026 viraltaco_ <https://viraltaco.com>
#ifndef vt_parallel_letter_frequency
#define vt_parallel_letter_frequency "com.viraltaco.letter-frequency v" "1.2.5"

#include <numeric>     // std::transform_reduce
#include <string_view> // std::string_view
#include <valarray>    // std::valarray
#include <array>       // std::array
#include <vector>      // std::vector

#ifndef __APPLE__
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ
#endif

namespace parallel_letter_frequency::inline v1_2_5 {
class frequency {
public:
  using string_view = typename std::string_view;
  using record_type = typename std::vector<string_view>;
  using key_type    = typename string_view::value_type;
  using size_type   = decltype (sizeof 0);
  using count_type  = size_type;

  class frequency_map {
  private:
    // MARK: Invariants
    static constexpr auto kAlphabetSize = 1zu + ('z' - 'a');
    using self_type  = typename std::valarray<count_type>;
    using value_type = typename self_type::value_type;

    // MARK: Members
    self_type self = std::valarray(count_type{}, kAlphabetSize);

  protected:
    // MARK Text Utils
    static constexpr auto upper = [](auto c) noexcept { return 'A' <= c and c <= 'Z'; };
    static constexpr auto lower = [](auto c) noexcept { return 'a' <= c and c <= 'z'; };
    static constexpr auto alpha = [](auto c) noexcept { return lower(c) or upper(c); };
    /// P: c is an ascii letter.
    /// R: The lexographic index (starting from 0 for 'a', ignoring case).
    [[nodiscard]] static constexpr auto index_of(const auto k) noexcept -> size_type {
      static constexpr auto kLookupTable = [] {
        auto arr = std::array<unsigned char, 128>{};
        for (unsigned char c{}; c != 128; ++c) {
          if      (lower(c)) arr[c] = c - 'a';
          else if (upper(c)) arr[c] = c - 'A';
          else               arr[c] = 0x7F; // 127
        }
        return arr;
      }();
      return kLookupTable[k & 0x7F]; //k & 0b0111'1111
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
    /// P: k is a valid key.
    /// R: the frequency value for the key.
    [[nodiscard]] auto operator [](const key_type k) const { return self[index_of(k)]; }
    [[nodiscard]] auto empty() const noexcept -> bool { return self.max() == 0zu; }

    auto insert(const string_view str) noexcept -> void {
      for (const auto k: str) {
        if (alpha(k)) { self[index_of(k)] += 1zu; }
      }
    }

    auto operator +=(frequency_map const& other) noexcept -> frequency_map& {
      self += other.self;
      return *this;
    }
  };

private:
  frequency_map self;

public:
  explicit frequency(record_type const& rec)
    : self(
      std::transform_reduce(PAR_UNSEQ                              // Execution policy
        rec.cbegin()                                               // Source start
      , rec.cend()                                                 // Source end
      , frequency_map{}                                            // Destination
      , [] (frequency_map a, frequency_map b) { return a += b; }   // Operation
      , [] (const string_view str) { return frequency_map{str}; }) // Transform
    )
  {}

  [[nodiscard]] auto empty() const noexcept -> bool { return self.empty(); }
  [[nodiscard]] auto operator [](const key_type k) const { return self[k]; }
};
}  // namespace parallel_letter_frequency::inline v1_2_5

namespace parallel_letter_frequency {
  namespace latest = v1_2_5;
} // namespace parallel_letter_frequency
#endif  // ndef vt_parallel_letter_frequency
