#ifndef LIBCPP__STD_REF_WRAPPER_HPP
#define LIBCPP__STD_REF_WRAPPER_HPP

#include <functional>
#include <type_traits>
#include <concepts>

namespace std {

template <class T>
inline constexpr bool is_ref_wrapper = false;

template <class T>
inline constexpr bool is_ref_wrapper<reference_wrapper<T>> = true;


template<class R, class T, class RQ, class TQ>
concept specialization_constraint = // CRW macro
    // C1. first type is a reference_wrapper.
    is_ref_wrapper<R> //  && R is cv-unqualified omitted for simplicit. it already is.
    // C2. T is cv-unqualified. omit same.
    // C3.common_reference with the underlying type exists
    && requires {    
        typename common_reference_t<typename R::type&, TQ>;
    }
    // C4. reference_wrapper as specified is convertible to the above
    && convertible_to<RQ, common_reference_t<typename R::type&, TQ>>
    ;

template <class R, class T, template <class> class RQual,  template <class> class TQual>
    requires(  specialization_constraint<R, T, RQual<R>, TQual<T>> 
           && !specialization_constraint<T, R, TQual<T>, RQual<R>>  )
struct basic_common_reference<R, T, RQual, TQual> {
    using type = common_reference_t<typename R::type&, TQual<T>>;
};


template <class T, class R, template <class> class TQual,  template <class> class RQual>
    requires(  specialization_constraint<R, T, RQual<R>, TQual<T>> 
           && !specialization_constraint<T, R, TQual<T>, RQual<R>>  )
struct basic_common_reference<T, R, TQual, RQual> {
    using type = common_reference_t<typename R::type&, TQual<T>>;
};

} // namespace std

#endif
