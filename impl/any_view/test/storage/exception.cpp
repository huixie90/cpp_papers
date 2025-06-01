#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>

#include "storage.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[storage exception]")

namespace {
template <bool MoveNoexcept, bool isBig>
struct MayThrow {
  int i;
  bool will_throw = false;

  static constexpr bool move_noexcept = MoveNoexcept;
  static constexpr std::size_t data_size = isBig ? 100z : 1z;
  char data[data_size]{};

  MayThrow(int ii, bool thr = false) : i(ii), will_throw(thr) {}
  MayThrow(const MayThrow& other) : i(other.i), will_throw(other.will_throw) {
    if (will_throw) {
      throw 5;
    }
  }

  MayThrow(MayThrow&& other) noexcept(MoveNoexcept)
      : i(other.i), will_throw(other.will_throw) {
    // modify moved from object
    other.i = -1;
    other.will_throw = false;
    if (!MoveNoexcept && will_throw) {
      throw 6;
    }
  }

  MayThrow& operator=(const MayThrow& other) {
    i = other.i;
    will_throw = other.will_throw;
    if (will_throw) {
      throw 7;
    }
    return *this;
  }

  MayThrow& operator=(MayThrow&& other) noexcept(MoveNoexcept) {
    i = other.i;
    will_throw = other.will_throw;
    other.i = -1;
    other.will_throw = false;
    if (!MoveNoexcept && will_throw) {
      throw 8;
    }
    return *this;
  }
};

using Storage = std::ranges::detail::storage<24, 8, true>;
using std::ranges::detail::type;

template <class T>
int get_i(const Storage& s) {
  return s.get_ptr<T>()->i;
}

template <class From, class To, bool Success, bool MoveCalled>
void test() {
  // move assign
  {
    Storage s1{type<From>{}, 5};
    Storage s2{type<To>{}, 6};
    if constexpr (!From::move_noexcept) {
      s1.get_ptr<From>()->will_throw = true;
    }
    if constexpr (!To::move_noexcept) {
      s2.get_ptr<To>()->will_throw = true;
    }
    try {
      s1 = std::move(s2);
      if constexpr (!Success) {
        REQUIRE(false);
      }
    } catch (int) {
    }

    if constexpr (!Success) {
      REQUIRE(get_i<From>(s1) == 5);
      REQUIRE(get_i<To>(s2) == 6);
    } else {
      REQUIRE(get_i<To>(s1) == 6);
      if constexpr (MoveCalled) {
        REQUIRE(get_i<To>(s2) == -1);
      } else {
        REQUIRE(s2.is_singular());
      }
    }
  }
}

using SmallNoexcept = MayThrow<true, false>;
using SmallThrow = MayThrow<false, false>;
using BigNoexcept = MayThrow<true, true>;
using BigThrow = MayThrow<true, true>;

static_assert(Storage::unittest_is_small<SmallNoexcept>());
static_assert(!Storage::unittest_is_small<SmallThrow>());
static_assert(!Storage::unittest_is_small<BigNoexcept>());
static_assert(!Storage::unittest_is_small<BigThrow>());

TEST_POINT("test noexcept") {
  test<SmallNoexcept, SmallNoexcept, true, true>();
  test<SmallThrow, SmallThrow, true, false>();
  test<BigNoexcept, BigNoexcept, true, false>();
  test<BigThrow, BigThrow, true, false>();
}

}  // namespace