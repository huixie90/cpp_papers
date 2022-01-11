#ifndef LIBCPP__RANGE_CONCAT_HPP
#define LIBCPP__RANGE_CONCAT_HPP

#include <concepts>
#include <functional>
#include <ranges>
#include <tuple>
#include <variant>

#include "utils.hpp"

namespace std::ranges {

namespace xo { // exposition only things (and persevering face)

// these concepts are needed if we make the implementation same as range-v3.
// for random access, range-v3 actually isn't constant time.
// For it + n, if n is bigger than the current view, it recursively
// goes to the end and checks the next view, which is O(N), where N
// is number of concated view.
// we can possibly use ranges::size if they are sized_range
template <bool Const, class... Views>
concept all_random_access = // exposition only
    (random_access_range<__maybe_const<Const, Views>>&&...);

template <bool Const, class... Views>
concept all_bidirectional = // exposition only
    (bidirectional_range<__maybe_const<Const, Views>>&&...);

template <bool Const, class... Views>
concept all_forward = // exposition only
    (forward_range<__maybe_const<Const, Views>>&&...);

template <bool Const, typename... Views>
constexpr auto iterator_concept_test() {
    if constexpr (all_random_access<Const, Views...>) {
        return random_access_iterator_tag{};
    } else if constexpr (all_bidirectional<Const, Views...>) {
        return bidirectional_iterator_tag{};
    } else if constexpr (all_forward<Const, Views...>) {
        return forward_iterator_tag{};
    } else {
        return input_iterator_tag{};
    }
}

template <typename... T>
using back = tuple_element_t<sizeof...(T) - 1, tuple<T...>>;

} // namespace xo

template <input_range... Views>
requires(view<Views>&&...) && (sizeof...(Views) > 0) class concat_view
    : public view_interface<concat_view<Views...>> {

    tuple<Views...> views_; // exposition only

    template <bool Const>
    class iterator {
        // use of exposition only trait `maybe-const` defined in
        // http://eel.is/c++draft/ranges#syn
        using ParentView = __maybe_const<Const, concat_view>;
        using BaseIt = variant<iterator_t<__maybe_const<Const, Views>>...>;

        // It is possible to not to store parent view pointer to make the view `borrowed_range`
        // if all the base views are `borrowed_range`. But we need to store all the begin
        // iterators and sentinels of the base views inside this iterator if they are copyable
        // not worth it I think.
        ParentView* parent_ = nullptr;
        BaseIt it_ = BaseIt();

        friend class iterator<!Const>;

      public:
        using difference_type = common_type_t<range_difference_t<__maybe_const<Const, Views>>...>;
        using value_type = common_type_t<range_value_t<__maybe_const<Const, Views>>...>;
        using iterator_concept = decltype(iterator_concept_test<Const, Views...>());

        /*
         * this one is tricky.
         * it depends on the iterate_category of every base and also depends on if
         * the common_reference_t<...> is actually a reference
         */
        // using iterator_category = ; // not always present.

        iterator() requires(default_initializable<iterator_t<__maybe_const<Const, Views>>>&&...) =
            default;

        template <class... Args>
        explicit iterator(ParentView& parent,
                          Args&&... args) requires constructible_from<BaseIt, Args&&...>
            : parent_{&parent}, it_{static_cast<Args&&>(args)...} {}

        constexpr iterator(iterator<!Const> i) requires Const &&
            (convertible_to<iterator_t<Views>, iterator_t<__maybe_const<Const, Views>>>&&...)
            : parent_{i.parent_}
            , it_{std::move(i.it_)} {}
    };


    // store only the sentinel of last view
    template <bool Const>
    class sentinel {
      public:
        sentinel() requires(default_initializable<iterator_t<__maybe_const<Const, xo::back<Views...>>>>) =
            default;
    };

  public:
    concat_view() requires(default_initializable<Views>&&...) = default;
    constexpr explicit concat_view(Views... views)
        : views_{static_cast<Views&&>(views)...} {}

    // used exposition only concepts simple-view defined here:
    // http://eel.is/c++draft/ranges#range.utility.helpers (we can reuse in the spec)
    // in clang it is __simple_view
    // question: should we check only the first view is not simple view or any view is not simple
    // view?
    constexpr auto begin() requires(!(__simple_view<Views> && ...)) {
        // return iterator<false>{...};
        // if the first N views are empty, we need to keep jumping the iterator
        // until we find the first view that is not empty and that is the begin iterator
        // This is not constant time (in terms of number of views) so we probably need to cache it.
        return (int*)(nullptr);
    }

    constexpr auto begin() const requires(range<const Views>&&...) {
        // return iterator<true>{...};
        // if the first N views are empty, we need to keep jumping the iterator
        // until we find the first view that is not empty and that is the begin iterator
        // This is not constant time (in terms of number of views) so we probably need to cache it.
        return (int*)(nullptr);
    }

    // question: should we check only the last view is not simple view or any view is not simple
    // view?
    constexpr auto end() requires(!(__simple_view<Views> && ...)) {
        // should we check only the last view or every view to be common_range?
        if constexpr ((common_range<Views> && ...)) {
            // return iterator<false>{...};
        } else {
            // return sentinel<false>{...};
        }
        return (int*)(nullptr);
    }

    // question: should we check only the last view or all the views?
    constexpr auto end() requires(range<const Views>&&...) {
        // should we check only the last view or every view to be common_range?
        if constexpr ((common_range<Views> && ...)) {
            // return iterator<false>{...};
        } else {
            // return sentinel<false>{...};
        }
        return (int*)(nullptr);
    }

    constexpr auto size() requires(sized_range<Views>&&...) {
        return apply(
            [](auto... sizes) {
                using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
                return (CT{0} + ... + CT{sizes});
            },
            concat_detail::tuple_transform(ranges::size, views_));
    }

    constexpr auto size() const requires(sized_range<const Views>&&...) {
        return apply(
            [](auto... sizes) {
                using CT = make_unsigned_t<common_type_t<decltype(sizes)...>>;
                return (CT{0} + ... + CT{sizes});
            },
            concat_detail::tuple_transform(ranges::size, views_));
    }
};

template <class... R>
concat_view(R&&...) -> concat_view<views::all_t<R>...>;

} // namespace std::ranges


#endif