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

/*
Benchmark                                                     Time             CPU      Time Old      Time New       CPU Old       CPU New
------------------------------------------------------------------------------------------------------------------------------------------
[BM_algo_vector vs. BM_algo_AnyView]/1024                  -0.8098         -0.8098          9615          1829          9614          1829
[BM_algo_vector vs. BM_algo_AnyView]/2048                  -0.8169         -0.8169         20651          3782         20649          3781
[BM_algo_vector vs. BM_algo_AnyView]/4096                  -0.8188         -0.8188         41035          7436         41032          7436
[BM_algo_vector vs. BM_algo_AnyView]/8192                  -0.8275         -0.8275         87227         15044         87220         15043
[BM_algo_vector vs. BM_algo_AnyView]/16384                 -0.8315         -0.8315        182922         30818        182908         30812
[BM_algo_vector vs. BM_algo_AnyView]/32768                 -0.8407         -0.8406        381383         60771        381342         60767
[BM_algo_vector vs. BM_algo_AnyView]/65536                 -0.8425         -0.8425        793920        125004        793846        124993
[BM_algo_vector vs. BM_algo_AnyView]/131072                -0.8656         -0.8656       1733654        232982       1733507        232964
[BM_algo_vector vs. BM_algo_AnyView]/262144                -0.8640         -0.8639       3592842        488714       3589708        488679
OVERALL_GEOMEAN                                            -0.8364         -0.8364             0             0             0             0
*/


static void BM_2algo_vector(benchmark::State& state) {
  UI ui{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  std::vector<std::string> widget_names;
  widget_names.reserve(ui.widgets_.size());
  for(const auto& widget : ui.widgets_) {
    widget_names.push_back(widget.name);
  }
  for (auto _ : state) {
    auto res = lib::algo1(widget_names);
    benchmark::DoNotOptimize(res);

  }
}
BENCHMARK(BM_2algo_vector)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

static void BM_2algo_AnyView(benchmark::State& state) {
  UI ui{global_widgets | std::views::take(state.range(0)) |
               std::ranges::to<std::vector>()};
  std::vector<std::string> widget_names;
  widget_names.reserve(ui.widgets_.size());
  for(const auto& widget : ui.widgets_) {
    widget_names.push_back(widget.name);
  }
  for (auto _ : state) {
    auto res = lib::algo2(std::views::all(widget_names));
    benchmark::DoNotOptimize(res);
  }
}
// Register the function as a benchmark
BENCHMARK(BM_2algo_AnyView)->RangeMultiplier(2)->Range(1 << 10, 1 << 18);

/*
Benchmark                                                       Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------------------
[BM_2algo_vector vs. BM_2algo_AnyView]/1024                  +3.4981         +3.4982           416          1873           416          1873
[BM_2algo_vector vs. BM_2algo_AnyView]/2048                  +3.1492         +3.1493           842          3495           842          3495
[BM_2algo_vector vs. BM_2algo_AnyView]/4096                  +3.6820         +3.6820          1671          7826          1671          7825
[BM_2algo_vector vs. BM_2algo_AnyView]/8192                  +3.2180         +3.2179          3527         14877          3527         14876
[BM_2algo_vector vs. BM_2algo_AnyView]/16384                 +3.5951         +3.5951          6782         31163          6781         31161
[BM_2algo_vector vs. BM_2algo_AnyView]/32768                 +3.3399         +3.3398         13525         58699         13525         58694
[BM_2algo_vector vs. BM_2algo_AnyView]/65536                 +3.3735         +3.3736         27040        118258         27037        118248
[BM_2algo_vector vs. BM_2algo_AnyView]/131072                +3.4739         +3.4739         55086        246454         55083        246434
[BM_2algo_vector vs. BM_2algo_AnyView]/262144                +3.6095         +3.6095        110593        509778        110585        509741
OVERALL_GEOMEAN                                              +3.4344         +3.4344             0             0             0             0
*/