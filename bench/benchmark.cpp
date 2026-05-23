#include <benchmark/benchmark.h>

#include <vector>
#include <string>
#include <string_view>
#include <limits>
#include <random>
#include <algorithm>

#include "../parallel_letter_frequency.h"
#include "includes/exercism/parallel_letter_frequency.h"
#include "includes/v1_1_1/parallel_letter_frequency.h"
#include "includes/v1_1_5/parallel_letter_frequency.h"
#include "includes/v1_1_6/parallel_letter_frequency.h"
#include "includes/v1_2_4/parallel_letter_frequency.h"
#include "includes/v2_0_0/parallel_letter_frequency.h"
#include "includes/v2_1_1/parallel_letter_frequency.h"
#include "includes/v3_1_0~M1/parallel_letter_frequency.h"

/// MARK: Config 
static constexpr auto kRangeMultiplier = 8zu;
static constexpr auto kRangeMin = 1zu << kRangeMultiplier;         // 1 << 10 = 1 024
static constexpr auto kRangeMax = kRangeMin << kRangeMultiplier;   // 1 024 << 10  = 1 048 576
static constexpr auto kNumString = 1024;

/// MARK: Fixture
struct ParallelLetterFrequencyFixture : public benchmark::Fixture {
 public:
  std::vector<std::string> texts;
  std::vector<std::string_view> views;

  void SetUp(benchmark::State const& state) override {
    texts.reserve(kNumString + 2);
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<> distrib(1, std::numeric_limits<char>::max());

    for (auto i = 0; i != kNumString; ++i) {
      const auto kLength = state.range(0);
      texts.emplace_back(kLength, 'x'); // 0x78
      std::generate_n(texts.back().begin(), kLength,
          [&rng, &distrib]() { return static_cast<char>(distrib(rng)); }
      );
      views.emplace_back(texts.back());
    }
  }

  void TearDown(benchmark::State const&) override {
    texts.clear();
    views.clear();
  }
};

/// MARK: Register macro
#define REGISTER_PL_BENCHMARK(version)                                   \
  BENCHMARK_DEFINE_F(ParallelLetterFrequencyFixture, version)            \
  (benchmark::State & state) {                                           \
    for (auto _ : state) {                                               \
      auto result = parallel_letter_frequency::version::frequency(views);\
      benchmark::DoNotOptimize(result);                                  \
    }                                                                    \
  }                                                                      \
  BENCHMARK_REGISTER_F(ParallelLetterFrequencyFixture, version)          \
      ->RangeMultiplier(kRangeMultiplier)                                \
      ->Range(kRangeMin, kRangeMax)

/// MARK: Benchmarks
REGISTER_PL_BENCHMARK(latest); // latest baseline
REGISTER_PL_BENCHMARK(v3_1_0_M1);

///MARK: slower than v3_0_0
REGISTER_PL_BENCHMARK(v2_0_0); // (second) baseline
REGISTER_PL_BENCHMARK(v2_1_1);

///MARK: slower than v2_0_0
REGISTER_PL_BENCHMARK(v1_1_5); // (old) baseline

///MARK: slower than v1_1_5
REGISTER_PL_BENCHMARK(exercism);
REGISTER_PL_BENCHMARK(v1_1_1);
REGISTER_PL_BENCHMARK(v1_1_6);
REGISTER_PL_BENCHMARK(v1_2_4);

/// MARK: UNDEFINE Register macro
#undef REGISTER_PL_BENCHMARK

BENCHMARK_MAIN();
