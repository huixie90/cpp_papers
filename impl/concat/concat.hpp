#ifndef LIBCPP__RANGE_CONCAT_HPP
#define LIBCPP__RANGE_CONCAT_HPP

#include <concepts>
#include <functional>
#include <ranges>
#include <tuple>

#include "utils.hpp"

namespace std::ranges {

template <input_range... Views>
requires(view<Views>&&...) && (sizeof...(Views) > 0) class concat_view
    : public view_interface<concat_view<Views...>> {

    tuple<Views...> views_ = tuple<Views...>();

    template <bool Const>
    class iterator;

    template <bool Const>
    class sentinel;

  public:
    concat_view() requires(default_initializable<Views>&&...) = default;
    constexpr explicit concat_view(Views... views)
        : views_{static_cast<Views&&>(views)...} {}

    // used exposition only concepts simple-view defined here:
    // http://eel.is/c++draft/ranges#range.utility.helpers
    // in clang it is __simple_view
    // question: should we check only the first view is not simple view or any view is not simple
    // view?
    constexpr auto begin() requires(!(__simple_view<Views> && ...)) {
        // return iterator<false>{...};
        return (int*)(nullptr);
    }

    constexpr auto begin() const requires(range<const Views>&&...) {
        // return iterator<true>{...};
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