
#include "ref_wrapper.hpp"
#include <catch2/catch_test_macros.hpp>

#include <concepts>

#define TEST_POINT(x) TEST_CASE(x, "[basics]")

template <class Result, class T1, class T2>
concept check = std::same_as<Result, std::common_reference_t<T1, T2>> &&
                std::same_as<Result, std::common_reference_t<T2, T1>>;

template <class T>
using Ref = std::reference_wrapper<T>;



// clang-format off

// cverf
static_assert(check<int &     , Ref<int      >, int &      >);
static_assert(check<int const&, Ref<int      >, int const& >);
static_assert(check<int const&, Ref<int const>, int &      >);
static_assert(check<int const&, Ref<int const>, int const& >);         
static_assert(check<const volatile int&, Ref<const volatile int>, const volatile int&>);         


// derived-base
struct B {};
struct D : B {};
struct C {
    operator B&() const;
};

// works out-of-the-box cause ?: is unambiguous
static_assert(check<B&      , Ref<D>,       B&      >);
static_assert(check<B const&, Ref<D>,       B const&>);
static_assert(check<B const&, Ref<D const>, B const&>);


// static_assert( check<Ref<B>      ,   Ref<B> ,      D&      >);
// static_assert( check<Ref<B>      ,   Ref<B>&,      D&      >); 
// static_assert( check<Ref<B const>,   Ref<B const>, D&      >);
// static_assert( check<B&          ,   Ref<D>,       B&      >); // ?: is not ambiguous
// static_assert( check<B           ,   Ref<D const>, B&      >); // ?: proper fails, so common_type is used
// static_assert(!check<B const&    ,   Ref<B>      , D const&>); // not even common_type, just fail. 
// 
// static_assert( check<Ref<B>      , Ref<B>      , C&      >);
// static_assert( check<Ref<B const>, Ref<B const>, C&      >);
// static_assert(!check<B           , Ref<C const>, B&      >); // fail common_type
// static_assert(!check<B const&    , Ref<B>      , C const&>); // fail common_type 



static_assert( check<B&      , Ref<B>      , D&      >);
static_assert( check<B&      , Ref<B>&     , D&      >); 
static_assert( check<B const&, Ref<B const>, D&      >);
static_assert( check<B const&, Ref<D const>, B&      >);
static_assert( check<B const&, Ref<B>      , D const&>);


static_assert( check<B&        , Ref<B>      , C&      >);
static_assert( check<B&        , Ref<B>      , C       >);
static_assert( check<B const&  , Ref<B const>, C       >);
static_assert(!check<B &       , Ref<C>      , B&      >); // Ref<C> cannot be converted to B&
static_assert( check<const B &       , Ref<B>      , C const&>); // ?: is ok


// clang-format on




TEST_POINT("recursive") {
    static_assert(
        std::same_as<std::common_reference_t<std::reference_wrapper<int>&,
                                             std::reference_wrapper<std::reference_wrapper<int>>>,
                     std::reference_wrapper<int>&>);
}
