
#include "ref_wrapper.hpp"
#include <catch2/catch_test_macros.hpp>

#include <concepts>

#define TEST_POINT(x) TEST_CASE(x, "[basics]")

TEST_POINT("simple") {
    static_assert(std::same_as<std::common_reference_t<int&, std::reference_wrapper<int>>, int&>);
}

TEST_POINT("recursive") {
    static_assert(
        std::same_as<std::common_reference_t<std::reference_wrapper<int>&,
                                             std::reference_wrapper<std::reference_wrapper<int>>>,
                     std::reference_wrapper<int>&>);
}