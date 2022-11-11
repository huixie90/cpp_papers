#pragma once

#include <concepts>
#include <type_traits>

template <class T>
concept HasType = requires {
    typename T::type;
};

template <class Result, class T1, class T2>
concept check1 = std::same_as<Result, std::common_reference_t<T1, T2>>;

template <class Result, class T1, class T2>
concept check2 = std::same_as<Result, std::common_reference_t<T2, T1>>;

template <class Result, class T1, class T2>
concept check = check1<Result, T1, T2> && check2<Result, T1, T2>;

template <class T1, class T2>
concept check_none1 = !HasType<std::common_reference<T1, T2>>;
template <class T1, class T2>
concept check_none2 = !HasType<std::common_reference<T2, T1>>;

template <class T1, class T2>
concept check_none = check_none1<T1, T2> && check_none2<T1, T2>;

template <class T>
using Ref = std::reference_wrapper<T>;

// https://eel.is/c++draft/meta.trans#other-2.4
template <class X, class Y>
using CondRes = decltype(false ? std::declval<X (&)()>()() : std::declval<Y (&)()>()());

template <class X, class Y>
struct Ternary {};

template <class X, class Y>
requires requires() { typename CondRes<X, Y>; }
struct Ternary<X, Y> {
    using type = CondRes<X, Y>;
};
template <class X, class Y>
using Ternary_t = typename Ternary<X, Y>::type;

// https://eel.is/c++draft/meta.trans#other-2.5
template <class A, class B>
using CopyCV =
#ifdef _MSC_VER
    std::_Copy_cv<A, B>;
#else
    std::__copy_cv<A, B>;
#endif

template <class A, class B>
using CommonRef =
#ifdef _MSC_VER
    std::_Common_reference2A<A, B>; // not quite but won't bother for now
#else
    std::__common_ref<A, B>;
// https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/std/type_traits#L3652
#endif
