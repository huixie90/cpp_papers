
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>

#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[input_common]")

namespace {

using AnyView = std::ranges::any_view<int, std::ranges::category::input |
                                                std::ranges::category::common>;
using Iter = std::ranges::iterator_t<AnyView>;

static_assert(std::input_iterator<Iter>);
static_assert(!std::forward_iterator<Iter>);
static_assert(std::same_as<std::iter_reference_t<Iter>, int&>);
static_assert(std::same_as<std::iter_rvalue_reference_t<Iter>, int&&>);
static_assert(std::same_as<std::iter_value_t<Iter>, int>);
static_assert(std::same_as<std::iter_difference_t<Iter>, ptrdiff_t>);

template <typename T>
concept has_iterator_category = requires() { typename T::iterator_category; };
static_assert(!has_iterator_category<Iter>);

constexpr void basic() {
  std::array a{1, 2, 3, 4, 5};
  AnyView v(std::views::all(a));

  Iter iter = v.begin();
  {
    std::same_as<int&> decltype(auto) r = *iter;
    assert(r == 1);
  }

  {
    std::same_as<Iter&> decltype(auto) r = ++iter;
    assert(*iter == 2);
    assert(&r == &iter);
  }

  {
    static_assert(std::same_as<decltype(iter++), void>);
    iter++;
    assert(*iter == 3);
  }

  {
    std::same_as<int&&> decltype(auto) r = std::ranges::iter_move(iter);
    assert(r == 3);
  }
}

constexpr void move() {
  std::array a{1, 2, 3, 4, 5};
  AnyView v(std::views::all(a));

  Iter iter1 = v.begin();
  Iter iter2(std::move(iter1));

  assert(*iter1 == 1);
  assert(*iter2 == 1);

  ++iter2;

  assert(*iter1 == 1);
  assert(*iter2 == 2);

  iter1 = std::move(iter2);
  assert(*iter1 == 2);
  assert(*iter2 == 2);
}

constexpr void copy() {
  std::array a{1, 2, 3, 4, 5};
  AnyView v(std::views::all(a));

  Iter iter1 = v.begin();
  Iter iter2(iter1);

  assert(*iter1 == 1);
  assert(*iter2 == 1);

  ++iter1;

  assert(*iter1 == 2);
  assert(*iter2 == 1);

  iter2 = iter1;
  assert(*iter1 == 2);
  assert(*iter2 == 2);
}

constexpr void equal() {
  std::array a{1, 2, 3, 4, 5};
  AnyView v(std::views::all(a));

  Iter iter1 = v.begin();
  Iter iter2 = v.begin();

  std::same_as<bool> decltype(auto) r = iter1 == iter2;
  assert(r);

  ++iter1;
  assert(iter1 != iter2);
}

constexpr bool test() {
  basic();
  move();
  copy();
  equal();
  return true;
}

TEST_POINT("input") {
  test();
  // static_assert(test());
}

}  // namespace
