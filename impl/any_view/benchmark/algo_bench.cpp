#include <benchmark/benchmark.h>

#include <random>
#include <ranges>
#include <vector>

#include "any_view.hpp"
#include "algo.hpp"

namespace {

struct Widget {
  std::string name;
  int size;
};

struct UI {
  std::vector<Widget> widgets_;
};


constexpr auto MaxSize = 1 << 18;

auto generate_random_widgets() {
  std::vector<Widget> widgets;
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
    widgets.push_back(Widget{gen_next_str(), gen_size()});
  }
  return widgets;
}

const auto global_widgets = generate_random_widgets();
}  // namespace


static void BM_algo_vector(benchmark::State& state) {
  UI ui{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    std::vector<std::string> widget_names;
    widget_names.reserve(ui.widgets_.size());
    for(const auto& widget : ui.widgets_) {
      widget_names.push_back(widget.name);
    }
    auto res = lib::algo1(widget_names);
    benchmark::DoNotOptimize(res);

  }
}
BENCHMARK(BM_algo_vector)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

static void BM_algo_AnyView(benchmark::State& state) {
  UI ui{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  for (auto _ : state) {
    auto res = lib::algo2(ui.widgets_ | std::views::transform( &Widget::name));
    benchmark::DoNotOptimize(res);
  }
}
// Register the function as a benchmark
BENCHMARK(BM_algo_AnyView)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

