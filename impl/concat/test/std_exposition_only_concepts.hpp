#pragma once

#include <concepts>
#include <type_traits>
#include <iterator>

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


//  +---------+
//  |  BONUS  |
//  +---------+

template <typename T>
concept has_iterator_category = requires() {
    typename T::iterator_category;
};

template <typename T>
concept has_value_type = requires() {
    typename T::value_type;
};

template <typename T>
using pointer_t = typename std::iterator_traits<T>::pointer;
