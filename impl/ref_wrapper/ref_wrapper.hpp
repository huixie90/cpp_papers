#ifndef LIBCPP__STD_REF_WRAPPER_HPP
#define LIBCPP__STD_REF_WRAPPER_HPP

#include <functional>
#include <type_traits>

namespace std {

template <class T>
inline constexpr bool is_ref_wrapper = false;

template <class T>
inline constexpr bool is_ref_wrapper<reference_wrapper<T>> = true;

template <class T>
    requires(!std::same_as<std::remove_cvref_t<T>, T>)
inline constexpr bool is_ref_wrapper<T> = is_ref_wrapper<std::remove_cvref_t<T>>;

template <class T, class U>
concept first_ref_wrapper_common_ref =
    is_ref_wrapper<T> &&
    requires { typename common_reference<typename std::remove_cvref_t<T>::type&, U>::type; } &&
    is_convertible_v<T, common_reference_t<typename std::remove_cvref_t<T>::type&, U>> &&
    is_convertible_v<U, common_reference_t<typename std::remove_cvref_t<T>::type&, U>>;

template <class T, class U>
concept second_ref_wrapper_common_ref = first_ref_wrapper_common_ref<U, T>;

template <class T, class U, template <class> class TQual, template <class> class UQual>
    requires(!first_ref_wrapper_common_ref<TQual<T>, UQual<reference_wrapper<U>>> &&
             second_ref_wrapper_common_ref<TQual<T>, UQual<reference_wrapper<U>>>)
struct basic_common_reference<T, reference_wrapper<U>, TQual, UQual> {
    using type = common_reference_t<TQual<T>, U&>;
};

template <class T, class U, template <class> class TQual, template <class> class UQual>
    requires(first_ref_wrapper_common_ref<TQual<reference_wrapper<T>>, UQual<U>> &&
             !second_ref_wrapper_common_ref<TQual<reference_wrapper<T>>, UQual<U>>)
struct basic_common_reference<reference_wrapper<T>, U, TQual, UQual> {
    using type = common_reference_t<T&, UQual<U>>;
};

} // namespace std

#endif