#ifndef LIBCPP__RANGE_CONCAT_UTILS_HPP
#define LIBCPP__RANGE_CONCAT_UTILS_HPP

#include <functional>
#include <utility>
#include <tuple>
#include <type_traits>

namespace std::ranges::concat_detail {

namespace tuple_or_pair_test {
template <typename T, typename U>
auto test() -> std::pair<T, U>;

template <typename... Ts>
requires(sizeof...(Ts) != 2) auto test() -> std::tuple<Ts...>;
} // namespace tuple_or_pair_test

// exposition only utilities from zip_view (zip_view is not implemented yet in libc++)
// http://eel.is/c++draft/ranges#range.zip.view
template <typename... Ts>
using tuple_or_pair = decltype(tuple_or_pair_test::test<Ts...>());

template <typename F, typename Tuple>
constexpr auto tuple_transform(F&& f, Tuple&& tuple) {
    return apply(
        [&]<class... Ts>(Ts && ... elements) {
            return tuple_or_pair<invoke_result_t<F&, Ts>...>(
                invoke(f, std::forward<Ts>(elements))...);
        },
        std::forward<Tuple>(tuple));
}


} // namespace std::ranges::concat_detail

#endif