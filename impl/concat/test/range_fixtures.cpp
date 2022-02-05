#include "range_fixtures.hpp"
#include "std_exposition_only_concepts.hpp"

#include <catch2/catch_test_macros.hpp>

#define TEST_POINT(x) TEST_CASE(x, "[range-fixtures]")



TEST_POINT("Cpp17OutputIter") {
    using It = Cpp17OutputIter;
    using ItCat = std::iterator_traits<It>::iterator_category;

    STATIC_CHECK(std::cpp17_iterator<It>);
    STATIC_CHECK(!std::cpp17_input_iterator<It>);
    STATIC_CHECK(std::same_as<ItCat, std::output_iterator_tag>);
    STATIC_CHECK(std::derived_from<ItCat, std::output_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::input_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::forward_iterator_tag>);

    STATIC_CHECK(!has_value_type<ItCat>);

    STATIC_CHECK(std::indirectly_writable<It, int>);
    STATIC_CHECK(!std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(std::incrementable<It>);
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(!std::input_iterator<It>);
    STATIC_CHECK(std::output_iterator<It, int>);
    STATIC_CHECK(!std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}


TEST_POINT("Cpp17InputIter") {
    using It = Cpp17InputIter;
    using ItCat = std::iterator_traits<It>::iterator_category;

    STATIC_CHECK(std::cpp17_iterator<It>);
    STATIC_CHECK(std::cpp17_input_iterator<It>);
    STATIC_CHECK(std::same_as<ItCat, std::input_iterator_tag>);

    STATIC_CHECK(std::derived_from<ItCat, std::input_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::output_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::forward_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::bidirectional_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::random_access_iterator_tag>);

    STATIC_CHECK(!std::indirectly_writable<It, int>);
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(std::incrementable<It>);
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(!std::output_iterator<It, int>);
    STATIC_CHECK(!std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}



TEST_POINT("Cpp17ForwardIter") {
    using It = Cpp17ForwardIter;
    using ItCat = std::iterator_traits<It>::iterator_category;

    STATIC_CHECK(std::cpp17_iterator<It>);
    STATIC_CHECK(std::cpp17_input_iterator<It>);
    STATIC_CHECK(std::same_as<ItCat, std::forward_iterator_tag>);

    STATIC_CHECK(std::derived_from<ItCat, std::input_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::output_iterator_tag>);
    STATIC_CHECK(std::derived_from<ItCat, std::forward_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::bidirectional_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::random_access_iterator_tag>);

    STATIC_CHECK(!std::indirectly_writable<It, int>);
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(std::incrementable<It>);
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(!std::output_iterator<It, int>);
    STATIC_CHECK(std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}


TEST_POINT("Cpp17ForwardMutableIter") {
    using It = Cpp17ForwardMutableIter;
    using ItCat = std::iterator_traits<It>::iterator_category;

    STATIC_CHECK(std::cpp17_iterator<It>);
    STATIC_CHECK(std::cpp17_input_iterator<It>);
    STATIC_CHECK(std::same_as<ItCat, std::forward_iterator_tag>);

    STATIC_CHECK(std::derived_from<ItCat, std::input_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::output_iterator_tag>); // what'd be correct?
    STATIC_CHECK(std::derived_from<ItCat, std::forward_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::bidirectional_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::random_access_iterator_tag>);

    STATIC_CHECK(std::indirectly_writable<It, int>);
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(std::incrementable<It>);
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(std::output_iterator<It, int>);
    STATIC_CHECK(std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}



TEST_POINT("Cpp17BidirectionalIter") {
    using It = Cpp17BidirectionalIter;
    using ItCat = std::iterator_traits<It>::iterator_category;

    STATIC_CHECK(std::cpp17_iterator<It>);
    STATIC_CHECK(std::cpp17_input_iterator<It>);
    STATIC_CHECK(std::same_as<ItCat, std::bidirectional_iterator_tag>);

    STATIC_CHECK(std::derived_from<ItCat, std::input_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::output_iterator_tag>);
    STATIC_CHECK(std::derived_from<ItCat, std::forward_iterator_tag>);
    STATIC_CHECK(std::derived_from<ItCat, std::bidirectional_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::random_access_iterator_tag>);

    STATIC_CHECK(std::indirectly_writable<It, int>);
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(std::incrementable<It>);
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(std::output_iterator<It, int>);
    STATIC_CHECK(std::forward_iterator<It>);
    STATIC_CHECK(std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}


TEST_POINT("Cpp20InputIter") {
    using It = Cpp20InputIter;

    STATIC_CHECK(!has_iterator_category<std::iterator_traits<It>>);

    STATIC_CHECK(!std::cpp17_iterator<It>);

    STATIC_CHECK(!std::indirectly_writable<It, int>); // rvalue reference
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(!std::incrementable<It>); // void postfix
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(!std::output_iterator<It, int>);
    STATIC_CHECK(!std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}

TEST_POINT("Cpp20InputCpp17OutputIter") {
    using It = Cpp20InputCpp17OutputIter;

    STATIC_CHECK(
        std::same_as<std::iterator_traits<It>::iterator_category, std::output_iterator_tag>);

    STATIC_CHECK(std::cpp17_iterator<It>);
    STATIC_CHECK(!std::cpp17_input_iterator<It>);

    STATIC_CHECK(!std::indirectly_writable<It, int>); // rvalue reference
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(!std::incrementable<It>); // no It == It
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It::Sentinel, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(!std::output_iterator<It, int>);
    STATIC_CHECK(!std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}



TEST_POINT("Cpp20ForwardIter") {
    using It = Cpp20ForwardIter;

    STATIC_CHECK(
        std::same_as<std::iterator_traits<It>::iterator_category, std::input_iterator_tag>);

    STATIC_CHECK(std::cpp17_input_iterator<It>);
    STATIC_CHECK(!std::cpp17_forward_iterator<It>);

    STATIC_CHECK(!std::indirectly_writable<It, int>); // rvalue reference
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(std::incrementable<It>);
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(!std::output_iterator<It, int>);
    STATIC_CHECK(std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}


TEST_POINT("Cpp20ForwardIterWithTraits") {
    using It = Cpp20ForwardIterWithTraits;

    STATIC_CHECK(
        std::same_as<std::iterator_traits<It>::iterator_category, std::input_iterator_tag>);

    STATIC_CHECK(std::cpp17_input_iterator<It>);
    STATIC_CHECK(!std::cpp17_forward_iterator<It>);

    STATIC_CHECK(!std::indirectly_writable<It, int>); // rvalue reference
    STATIC_CHECK(std::indirectly_readable<It>);
    STATIC_CHECK(std::weakly_incrementable<It>);
    STATIC_CHECK(std::incrementable<It>);
    STATIC_CHECK(std::input_or_output_iterator<It>);
    STATIC_CHECK(std::sentinel_for<It, It>);
    STATIC_CHECK(std::input_iterator<It>);
    STATIC_CHECK(!std::output_iterator<It, int>);
    STATIC_CHECK(std::forward_iterator<It>);
    STATIC_CHECK(!std::bidirectional_iterator<It>);
    STATIC_CHECK(!std::random_access_iterator<It>);
}
