#include <benchmark/benchmark.h>

#include <ranges>
#include <vector>

#include "any_view.hpp"

static void BM_vector(benchmark::State& state) {
  std::vector v =
      std::views::iota(0, state.range(0)) | std::ranges::to<std::vector>();
  for (auto _ : state) {
    for (auto i : v) {
      benchmark::DoNotOptimize(i);
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_vector)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

// Define another benchmark
static void BM_AnyView(benchmark::State& state) {
  std::vector v =
      std::views::iota(0, state.range(0)) | std::ranges::to<std::vector>();
  std::ranges::any_view<int&> av(std::views::all(v));
  for (auto _ : state) {
    for (auto i : av) {
      benchmark::DoNotOptimize(i);
    }
  }
}
BENCHMARK(BM_AnyView)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);
