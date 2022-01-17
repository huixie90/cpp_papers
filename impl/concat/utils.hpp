#ifndef LIBCPP_RANGE_CONCAT_UTILS_HPP
#define LIBCPP_RANGE_CONCAT_UTILS_HPP

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
// http://eel.is/c++draft/ranges#range.zip.view (perhaps we can reuse in the spec)
// this paper proposed it to be moved out of zip_view
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2374r3.html
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



// Exposition only utilities, normalize cross-compiler:
#ifdef _MSC_VER
namespace std::ranges {
    template<bool C, typename T>
    using __maybe_const = _Maybe_const<C, T>;

    
    template <typename T>
    concept __simple_view = _Simple_view<T>;
}
#endif


#endif