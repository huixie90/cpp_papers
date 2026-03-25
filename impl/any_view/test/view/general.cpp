#include <algorithm>
#include <array>
#include <cassert>
#include <catch2/catch_test_macros.hpp>

#include "any_view.hpp"
#include "../helper.hpp"

#define TEST_POINT(x) TEST_CASE(x, "[general]")

// static_assert(std::ranges::view<std::ranges::common_view<std::ranges::any_view<int, std::ranges::any_view_options::forward>>>);

