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

struct ParallelLetterFrequencyFixture : public benchmark::Fixture {
 public:
  std::vector<std::string> texts;
  std::vector<std::string_view> views;

  void SetUp(const benchmark::State& state) override {
    std::mt19937_64 rng(42);
    std::uniform_int_distribution<> distrib(1, std::numeric_limits<char>::max());

    const int num_strings = 10;
    for (int i = 0; i < num_strings; ++i) {
      texts.emplace_back(state.range(0), 'x');
      std::generate_n(
          texts.back().begin(), texts.back().length(),
          [&rng, &distrib]() { return static_cast<char>(distrib(rng)); });
      views.emplace_back(texts.back());
    }
  }

  void TearDown(const benchmark::State& /*state*/) override {
    texts.clear();
    views.clear();
  }
};

static constexpr auto kRangeMultiplier = 10zu;
static constexpr auto kRangeMin = 1zu << kRangeMultiplier;
static constexpr auto kRangeMax = kRangeMin * kRangeMin * 32;
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

REGISTER_PL_BENCHMARK(v1_1_5);
REGISTER_PL_BENCHMARK(v1_2_4);
REGISTER_PL_BENCHMARK(latest);
// slower than v1_1_5
//REGISTER_PL_BENCHMARK(exercism);
//REGISTER_PL_BENCHMARK(v1_1_1);
//REGISTER_PL_BENCHMARK(v1_1_6);

#undef REGISTER_PL_BENCHMARK

BENCHMARK_MAIN();
