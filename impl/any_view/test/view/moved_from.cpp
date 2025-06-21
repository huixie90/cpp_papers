#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <ranges>

#include "../helper.hpp"
#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[moved_from]")

namespace {

using AnyView =
    std::ranges::any_view<int, std::ranges::any_view_options::forward>;

[[maybe_unused]] constexpr void small_buffer() {
  std::array arr{1, 2, 3, 4, 5};

  AnyView view1(arr);
  AnyView view2(std::move(view1));

  assert(*view2.begin() == 1);

  assert(std::ranges::empty(view1));
  for (const auto& elem : view1) {
    assert(false);
    (void)elem;
  }

  view1 = arr;
  assert(*view1.begin() == 1);
}

struct BigView : std::ranges::view_base {
  int* data;
  std::size_t size;

  std::array<char, 1024> stuff{};

  constexpr BigView(int* d, std::size_t s) : data(d), size(s) {}

  constexpr auto begin() const { return data; }
  constexpr auto end() const { return data + size; }
};

constexpr void heap() {
  std::array arr{1, 2, 3, 4, 5};
  BigView big_view(arr.data(), arr.size());
  AnyView view1(big_view);
  AnyView view2(std::move(view1));

  assert(*view2.begin() == 1);

  assert(std::ranges::empty(view1));
  for (const auto& elem : view1) {
    assert(false);
    (void)elem;
  }

  view1 = arr;
  assert(*view1.begin() == 1);
}

constexpr bool test() {
  small_buffer();
  heap();
  return true;
}

TEST_POINT("moved_from") {
  test();
  static_assert(test());
}

}  // namespace
