#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <string>
#include <type_traits>

#include "../helper.hpp"
#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[conversion]")
namespace {

static_assert(
    std::is_constructible_v<std::ranges::any_view<int>, ArrView<int*>>);

static_assert(
    std::is_constructible_v<std::ranges::any_view<const int>, ArrView<int*>>);
static_assert(
    !std::is_constructible_v<std::ranges::any_view<int>, ArrView<const int*>>);

constexpr void test_ref_conv() {
  int a[] = {1, 2, 3, 4, 5};
  ArrView<int*> av(a);
  std::ranges::any_view<const int> v(std::views::all(a));
  auto it = v.begin();
  std::same_as<const int&> decltype(auto) r = *it;

  assert(r == 1);
  ++it;
  assert(*it == 2);
}

static_assert(std::is_constructible_v<
              std::ranges::any_view<std::pair<int, int>,
                                    std::ranges::any_view_options::input,
                                    std::pair<int&, int&>>,
              ArrView<std::pair<int&, int&>*>>);

static_assert(!std::is_constructible_v<
                  std::ranges::any_view<std::pair<MoveOnly, MoveOnly>,
                                        std::ranges::any_view_options::input,
                                        std::pair<MoveOnly&, MoveOnly&>>,
                  ArrView<std::pair<MoveOnly&, MoveOnly&>*>>,
              "value_type is not convertible");

static_assert(!std::is_constructible_v<
                  std::ranges::any_view<
                      std::pair<int, int>, std::ranges::any_view_options::input,
                      std::pair<int&, int&>, std::pair<int&&, int&&>>,
                  ArrView<std::pair<int&, int&>*>>,
              "rvalue reference is not convertible");

constexpr bool test() {
  test_ref_conv();
  return true;
}

TEST_POINT("conversion") {
  test();
  static_assert(test());
}

}  // namespace