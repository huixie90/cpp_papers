#include "range_fixtures.hpp"

#include <concepts>
#include <type_traits>
#include <iterator>

#include <catch2/catch_test_macros.hpp>

#define TEST_POINT(x) TEST_CASE(x, "[range-fixtures]")


namespace std {

// from: http://eel.is/c++draft/iterator.synopsis#concept:can-reference
template <class T>
using with_reference = T&; // exposition only
template <class T>
concept can_reference // exposition only
    = requires {
    typename with_reference<T>;
};

// from: http://eel.is/c++draft/iterator.traits
template <class I>
concept cpp17_iterator = requires(I i) {
    { *i } -> can_reference;
    { ++i } -> same_as<I&>;
    { *i++ } -> can_reference;
}
&&copyable<I>;

template <class I>
concept cpp17_input_iterator = cpp17_iterator<I> && equality_comparable<I> && requires(I i) {
    typename incrementable_traits<I>::difference_type;
    typename indirectly_readable_traits<I>::value_type;
    typename common_reference_t<iter_reference_t<I>&&,
                                typename indirectly_readable_traits<I>::value_type&>;
    typename common_reference_t<decltype(*i++)&&,
                                typename indirectly_readable_traits<I>::value_type&>;
    requires signed_integral<typename incrementable_traits<I>::difference_type>;
};


template <class I>
concept cpp17_forward_iterator = cpp17_input_iterator<I> &&                    //
                                 constructible_from<I> &&                      //
                                 is_lvalue_reference_v<iter_reference_t<I>> && //
                                 same_as < remove_cvref_t<iter_reference_t<I>>,
typename indirectly_readable_traits<I>::value_type > && //
                                                     requires(I i) {
    { i++ } -> convertible_to<const I&>;
    { *i++ } -> same_as<iter_reference_t<I>>;
};


template <class I>
concept cpp17_bidirectional_iterator = cpp17_forward_iterator<I> && requires(I i) {
    { --i } -> same_as<I&>;
    { i-- } -> convertible_to<const I&>;
    { *i-- } -> same_as<iter_reference_t<I>>;
};

template <class I>
concept cpp17_random_access_iterator = cpp17_bidirectional_iterator<I> && totally_ordered<I> &&
    requires(I i, typename incrementable_traits<I>::difference_type n) {
    { i += n } -> same_as<I&>;
    { i -= n } -> same_as<I&>;
    { i + n } -> same_as<I>;
    { n + i } -> same_as<I>;
    { i - n } -> same_as<I>;
    { i - i } -> same_as<decltype(n)>;
    { i[n] } -> convertible_to<iter_reference_t<I>>;
};
} // namespace std



TEST_POINT("Cpp17OutputIter") {
    using It = Cpp17OutputIter;
    using ItCat = std::iterator_traits<It>::iterator_category;

    STATIC_CHECK(std::cpp17_iterator<It>);
    STATIC_CHECK(!std::cpp17_input_iterator<It>);
    STATIC_CHECK(std::same_as<ItCat, std::output_iterator_tag>);
    STATIC_CHECK(std::derived_from<ItCat, std::output_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::input_iterator_tag>);
    STATIC_CHECK(!std::derived_from<ItCat, std::forward_iterator_tag>);


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



template <typename T>
concept has_iterator_category = requires() {
    typename T::iterator_category;
};


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
