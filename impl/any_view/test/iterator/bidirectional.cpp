
#include <array>
#include <catch2/catch_test_macros.hpp>

#include "any_view.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[bidirectional]")

namespace {

using AnyView =
    std::ranges::any_view<int, std::ranges::any_view_options::bidirectional>;
using Iter = std::ranges::iterator_t<AnyView>;

static_assert(std::bidirectional_iterator<Iter>);
static_assert(!std::random_access_iterator<Iter>);
static_assert(std::same_as<std::iter_reference_t<Iter>, int&>);
static_assert(std::same_as<std::iter_rvalue_reference_t<Iter>, int&&>);
static_assert(std::same_as<std::iter_value_t<Iter>, int>);
static_assert(std::same_as<std::iter_difference_t<Iter>, ptrdiff_t>);
static_assert(
    std::same_as<typename std::iterator_traits<Iter>::iterator_category,
                 std::bidirectional_iterator_tag>);
static_assert(
    std::same_as<typename std::iterator_traits<Iter>::value_type, int>);
static_assert(
    std::same_as<typename std::iterator_traits<Iter>::reference, int&>);
static_assert(std::same_as<typename std::iterator_traits<Iter>::difference_type,
                           ptrdiff_t>);

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
    std::same_as<Iter> decltype(auto) r = iter++;
    assert(*iter == 3);
    assert(*r == 2);
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

  if !consteval {
    assert(*iter1 == 1);
  }
  assert(*iter2 == 1);

  ++iter2;

  if !consteval {
    assert(*iter1 == 1);
  }
  assert(*iter2 == 2);

  iter1 = std::move(iter2);
  assert(*iter1 == 2);
  if !consteval {
    assert(*iter2 == 2);
  }
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
  Iter iter2(iter1);

  std::same_as<bool> decltype(auto) r = iter1 == iter2;
  assert(r);

  ++iter1;
  assert(iter1 != iter2);
}

constexpr void decrement() {
  std::array a{1, 2, 3, 4, 5};
  AnyView v(std::views::all(a));

  Iter iter = v.begin();
  ++iter;
  ++iter;
  ++iter;

  assert(*iter == 4);

  {
    std::same_as<Iter&> decltype(auto) r = --iter;
    assert(*iter == 3);
    assert(&r == &iter);
  }

  {
    std::same_as<Iter> decltype(auto) r = iter--;
    assert(*iter == 2);
    assert(*r == 3);
  }
}

constexpr bool test() {
  basic();
  move();
  copy();
  equal();
  decrement();
  return true;
}

TEST_POINT("bidirectional") {
  test();
  static_assert(test());
}

}  // namespace
