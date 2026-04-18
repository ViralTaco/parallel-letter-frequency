#include <benchmark/benchmark.h>

#include <algorithm>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "../parallel_letter_frequency.h"
#include "includes/exercism/parallel_letter_frequency.h"
#include "includes/v1_1_1/parallel_letter_frequency.h"
#include "includes/v1_1_5/parallel_letter_frequency.h"
#include "includes/v1_1_6/parallel_letter_frequency.h"

struct ParallelLetterFrequencyFixture : public benchmark::Fixture {
 public:
  std::vector<std::string> texts;
  std::vector<std::string_view> views;

  void SetUp(const benchmark::State& state) override {
    std::mt19937 rng(42);
    std::uniform_int_distribution<> distrib(32, 126);

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

static constexpr auto kRangeMultiplier = 10u;
static constexpr auto kRangeMin = 1u << kRangeMultiplier;
static constexpr auto kRangeMax = kRangeMin << kRangeMultiplier;
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

REGISTER_PL_BENCHMARK(exercism);
REGISTER_PL_BENCHMARK(v1_1_1);
REGISTER_PL_BENCHMARK(v1_1_5);
REGISTER_PL_BENCHMARK(v1_1_6);
REGISTER_PL_BENCHMARK(latest);

#undef REGISTER_PL_BENCHMARK

BENCHMARK_MAIN();
