#pragma once /// Copyright 2026 viraltaco_ <https://viraltaco.com>
#ifndef vt_parallel_letter_frequency_v1_1_1
#define vt_parallel_letter_frequency_v1_1_1 "com.viraltaco.letter-frequency v" "1.1.1"

#include <numeric>     // std::transform_reduce
#include <stdexcept>   // std::out_of_range
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <valarray>    // std::valarray
#include <vector>      // std::vector

#ifndef __APPLE__
#include <execution>
#define PAR_UNSEQ std::execution::par_unseq,
#else
#define PAR_UNSEQ
#endif

namespace parallel_letter_frequency::inline v1_1_1 {
class frequency {
public:
  using string_view = typename std::string_view;
  using record_type = typename std::vector<string_view>;
  using key_type    = typename string_view::value_type;
  using count_type  = unsigned long;

private:
  class frequency_map {
  private:
    // MARK: Invariants
    static constexpr auto kAlphabetSize = 1zu + 'z' - 'a';
    using self_type  = typename std::valarray<count_type>;
    using value_type = typename self_type::value_type;
    using node_type  = value_type&;
    using size_type  = decltype (sizeof 0);
    
    // MARK: Members
    self_type self;
    bool zero = true;
  protected:
    // MARK Text Utils
    static constexpr auto upper = [](auto c) noexcept { return 'A' <= c and c <= 'Z'; };
    static constexpr auto lower = [](auto c) noexcept { return 'a' <= c and c <= 'z'; };
    static constexpr auto alpha = [](auto c) noexcept { return lower(c) or upper(c); };
    /// P: c is an ascii letter.
    /// R: The lexographic index (starting from 0 for 'a', ignoring case).
    [[nodiscard]] static constexpr auto index_for(const auto c) noexcept -> size_type {
      return static_cast<size_type> (c - (upper(c) ? 'A' : 'a'));
    }

  public:
    // MARK: Rule of Five
    frequency_map() noexcept
      : self(count_type{0}, kAlphabetSize)
    {}
    
    frequency_map(const string_view str) noexcept 
      : self(count_type{0}, kAlphabetSize)
    { this->insert(str); }

    frequency_map(frequency_map&& map) noexcept = default;
    frequency_map& operator=(frequency_map&& map) noexcept = default;

    frequency_map(frequency_map const& map) noexcept = default;
    frequency_map& operator=(frequency_map const& map) noexcept = default;
    
    // MARK: methods
    /// P: k is a valid key.
    /// R: the frequency value for the key.
    auto operator [](const key_type k) const { return self[index_for(k)]; }
    auto empty() const noexcept -> bool { return zero; }
    
    auto insert(const string_view str) noexcept -> void {
      for (const auto k : str) {
        if (not alpha(k)) continue;  // Not a letter: skip
        self[index_for(k)] += 1;
        zero = false;
      }
    }
    auto combine(frequency_map const& other) noexcept -> frequency_map& {
      if (not other.zero) {
        self += other.self;
        zero = false;
      }
      return *this;
    }
    auto operator +=(frequency_map const& other) noexcept -> frequency_map& {
      return combine(other);
    }
  };

  frequency_map self;

public:
  frequency(record_type const& rec)
    : self(
      std::transform_reduce(PAR_UNSEQ                              // Execution policy
        rec.cbegin()                                               // Source start
      , rec.cend()                                                 // Source end
      , frequency_map{}                                            // Destination
      , [] (frequency_map a, frequency_map b) { return a += b; }   // Operation
      , [] (const string_view str) { return frequency_map{str}; }) // Transform
    )
  {}

  auto empty() const noexcept -> bool { return self.empty(); }
  auto operator [](const key_type k) const { return self[k]; }
};
}  // namespace parallel_letter_frequency

#endif  // ndef vt_parallel_letter_frequency
