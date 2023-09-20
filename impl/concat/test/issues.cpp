
#include <catch2/catch_test_macros.hpp>

#include "concat.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[issues]")

TEST_POINT("#4") {
  std::ranges::concat_view cv{std::vector{1, 2}, std::vector{3, 4}};

  auto it = cv.begin();
  auto cit = std::as_const(cv).begin();

  [[maybe_unused]] decltype(it) it_copy(it);     // OK
  [[maybe_unused]] decltype(cit) cit_copy(cit);  // OK
  // [[maybe_unused]] decltype(it) it_copy2(cit);   // NOK by design
  [[maybe_unused]] decltype(cit) cit_copy2(it);  // NOK  bug!
}
