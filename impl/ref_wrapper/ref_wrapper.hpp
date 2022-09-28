#ifndef LIBCPP__STD_REF_WRAPPER_HPP
#define LIBCPP__STD_REF_WRAPPER_HPP

#include <functional>
#include <type_traits>

namespace std {

template <class T, template <class> class TQual, template <class> class UQual>
struct basic_common_reference<T, reference_wrapper<T>, TQual, UQual> {
    using type = common_reference_t<TQual<T>, T&>;
};

template <class T, template <class> class TQual, template <class> class UQual>
struct basic_common_reference<reference_wrapper<T>, T, TQual, UQual> {
    using type = common_reference_t<UQual<T>, T&>;
};

} // namespace std

#endif