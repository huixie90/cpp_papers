
#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <type_traits>

#include "any_view.hpp"
#include "../helper.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[bidirectional_view]")

namespace {

using AnyView =
    std::ranges::any_view<int,
                          std::ranges::any_view_options::bidirectional>;

static_assert(std::ranges::bidirectional_range<AnyView>);
static_assert(!std::ranges::random_access_range<AnyView>);
static_assert(std::movable<AnyView>);
static_assert(!std::copyable<AnyView>);
static_assert(!std::ranges::sized_range<AnyView>);
static_assert(!std::ranges::common_range<AnyView>);
static_assert(!std::ranges::borrowed_range<AnyView>);

using AnyViewFull =
    std::ranges::any_view<int, std::ranges::any_view_options::bidirectional |
                                   std::ranges::any_view_options::sized |
                                   std::ranges::any_view_options::copyable |
                                   std::ranges::any_view_options::borrowed>;

static_assert(std::ranges::bidirectional_range<AnyViewFull>);
static_assert(!std::ranges::random_access_range<AnyViewFull>);
static_assert(std::movable<AnyViewFull>);
static_assert(std::copyable<AnyViewFull>);
static_assert(std::ranges::sized_range<AnyViewFull>);
static_assert(!std::ranges::common_range<AnyViewFull>);
static_assert(std::ranges::borrowed_range<AnyViewFull>);

static_assert(!std::is_constructible_v<AnyView, InputView>);
static_assert(!std::is_constructible_v<AnyView, ForwardView>);
static_assert(std::is_constructible_v<AnyView, BidiView>);
static_assert(std::is_constructible_v<AnyView, RandomAccessView>);
static_assert(std::is_constructible_v<AnyView, ContiguousView>);

template <class V>
constexpr void basic() {
  std::array v{1, 2, 3, 4, 5};

  V view(std::views::all(v));

  auto it = view.begin();
  assert(*it == 1);

  auto st = view.end();
  assert(it != st);

  ++it;
  ++it;
  ++it;
  ++it;
  ++it;

  assert(it == st);
}

template <class V>
constexpr void forward() {
  std::array v{1, 2, 3, 4, 5};
  V view(std::views::all(v));
  auto it1 = view.begin();
  ++it1;
  assert(*it1 == 2);

  auto it2 = view.begin();
  assert(*it2 == 1);

  it2 = it1;
  assert(*it2 == 2);
}

template <class V>
constexpr void bidirectional() {
  std::array v{1, 2, 3, 4, 5};
  V view(std::views::all(v));
  auto it1 = view.begin();
  ++it1;
  assert(*it1 == 2);

  --it1;
  assert(*it1 == 1);
}

template <class V>
constexpr void move() {
  std::array v1{1, 2, 3, 4, 5};
  std::array v2{3};

  V view1(std::views::all(v1));
  V view2(std::views::all(v2));

  V view3(std::move(view1));

  assert(*view3.begin() == 1);

  view3 = std::move(view2);
  assert(*view3.begin() == 3);
}

template <class V>
constexpr void copy() {
  std::array v1{1, 2, 3, 4, 5};
  std::array v2{3};

  V view1(std::views::all(v1));
  V view2(std::views::all(v2));

  V view3(view1);

  assert(*view1.begin() == 1);
  assert(*view3.begin() == 1);

  view3 = view2;
  assert(*view2.begin() == 3);
  assert(*view3.begin() == 3);
}

template <class V>
constexpr void size() {
  std::array v1{1, 2, 3, 4, 5};

  V view1(std::views::all(v1));
  assert(view1.size() == 5);
}

constexpr bool test() {
  basic<AnyView>();
  basic<AnyViewFull>();
  forward<AnyView>();
  forward<AnyViewFull>();
  bidirectional<AnyView>();
  bidirectional<AnyViewFull>();
  move<AnyView>();
  move<AnyViewFull>();
  copy<AnyViewFull>();
  size<AnyViewFull>();
  return true;
}

TEST_POINT("forward") {
  test();
  static_assert(test());
}

}  // namespace
