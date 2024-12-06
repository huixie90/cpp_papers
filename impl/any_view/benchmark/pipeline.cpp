#include <benchmark/benchmark.h>

#include <random>
#include <ranges>
#include <vector>

#include "any_view.hpp"
#include "widget.hpp"

namespace {

constexpr auto MaxSize = 1 << 18;

auto generate_random_widgets() {
  std::vector<lib::Widget> widgets;
  widgets.reserve(MaxSize);

  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::random_device char_dev;
  std::mt19937 char_rng(char_dev());
  std::uniform_int_distribution<std::mt19937::result_type> char_dist(
      0, sizeof(alphanum) - 1);

  std::random_device len_dev;
  std::mt19937 len_rng(len_dev());
  std::uniform_int_distribution<std::mt19937::result_type> len_dist(1, 30);

  auto gen_next_str = [&]() {
    int len = len_dist(len_rng);
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
      tmp_s.push_back(alphanum[char_dist(char_rng)]);
    }

    return tmp_s;
  };

  std::random_device w_dev;
  std::mt19937 w_rng(w_dev());
  std::uniform_int_distribution<int> w_dist(0, 100);

  auto gen_size = [&] { return w_dist(w_rng); };

  for (auto i = 0; i < MaxSize; ++i) {
    widgets.push_back(lib::Widget{gen_next_str(), gen_size()});
  }
  return widgets;
}

const auto global_widgets = generate_random_widgets();
}  // namespace

using namespace lib;

static void BM_AnyViewPipeline(benchmark::State& state) {
  lib::UI1 ui1{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    for (auto const& name : ui1.getWidgetNames()) {
      benchmark::DoNotOptimize(const_cast<std::string&>(name));
    }
  }
}
BENCHMARK(BM_AnyViewPipeline)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

static void BM_RawPipeline(benchmark::State& state) {
  lib::UI2 ui2{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    for (const auto& name : ui2.getWidgetNames()) {
      benchmark::DoNotOptimize(const_cast<std::string&>(name));
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_RawPipeline)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

static void BM_VectorCopy(benchmark::State& state) {
  lib::UI3 ui3{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    for (const auto& name : ui3.getWidgetNames()) {
      benchmark::DoNotOptimize(const_cast<std::string&>(name));
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_VectorCopy)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

static void BM_VectorCopyReserve(benchmark::State& state) {
  lib::UI3B ui3b{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    for (const auto& name : ui3b.getWidgetNames()) {
      benchmark::DoNotOptimize(const_cast<std::string&>(name));
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_VectorCopyReserve)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

static void BM_VectorCopyRanges(benchmark::State& state) {
  lib::UI3C ui3c{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    for (const auto& name : ui3c.getWidgetNames()) {
      benchmark::DoNotOptimize(const_cast<std::string&>(name));
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_VectorCopyRanges)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

static void BM_VectorRefWrapper(benchmark::State& state) {
  lib::UI4 ui4{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    for (const auto& nameRef : ui4.getWidgetNames()) {
      benchmark::DoNotOptimize(const_cast<std::string&>(nameRef.get()));
    }
  }
}
// Register the function as a benchmark
BENCHMARK(BM_VectorRefWrapper)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);
