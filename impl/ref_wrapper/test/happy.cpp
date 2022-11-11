#include "ref_wrapper.hpp"
#include <catch2/catch_test_macros.hpp>

#define TEST_POINT(x) TEST_CASE(x, "[happy]")


//   +-----------------------------------+
//   |            TESTS                  |
//   +-----------------------------------+


// clang-format off
static_assert(check<int &     , Ref<int      >, int &      >);
static_assert(check<int const&, Ref<int      >, int const& >);
static_assert(check<int const&, Ref<int const>, int &      >);
static_assert(check<int const&, Ref<int const>, int const& >);         
static_assert(check<const volatile int&, Ref<const volatile int>, const volatile int&>);         

static_assert(!HasType< Ternary<Ref<int> const &, int&> >);  // ok, ternary is still ambiguous
static_assert(std::convertible_to<Ref<int> const &, int &>); // Ref<int> const& CAN be converted to int&
static_assert(check<int const&, Ref<int> const &, int &  >); // BUT we compute common ref as const int &
static_assert(std::same_as<int const &, CommonRef<Ref<int> const &, int &>> ); // BECAUSE of this first rule of common_reference


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


// Do implicit convertable
struct Y {
    operator B& () const;
};
struct X {
    operator B& () const;
};

static_assert( std::same_as< B&, CondRes<X,B&> > );
static_assert( std::same_as< B&, CondRes<Y,B&> > );
static_assert( std::same_as< B&, CondRes<X const &,B&> > );
static_assert( std::same_as< B&, CondRes<Y const &,B&> > );


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
    using Ri = Ref<int>;
    using RRi = Ref<Ref<int>>;
    using RRRi = Ref<Ref<Ref<int>>>;
    static_assert(check<Ri&, Ri&, RRi>);
    static_assert(check<RRi&, RRi&, RRRi>);
    static_assert(check<Ri, Ri, RRi>);
    static_assert(check<RRi, RRi, RRRi>);

    static_assert(check_none<int&, RRi>);
    static_assert(check_none<int, RRi>);
    static_assert(check_none<int&, RRRi>);
    static_assert(check_none<int, RRRi>);

    static_assert(check_none<Ri&, RRRi>);
    static_assert(check_none<Ri, RRRi>);
}

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    // your setup ...

    int result = Catch::Session().run(argc, argv);

    // your clean-up...

    return result;
}