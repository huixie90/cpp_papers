#pragma once

#include <type_traits>
#include <utility>

namespace std {

#if 1 || __cpp_lib_reference_from_temporary >= 202202
#else
template <class _Tp, class _Up>
struct reference_converts_from_temporary
    : public bool_constant<__reference_converts_from_temporary(_Tp, _Up)> {};

template <class _Tp, class _Up>
inline constexpr bool reference_converts_from_temporary_v =
    __reference_converts_from_temporary(_Tp, _Up);

#endif



}  // namespace std
