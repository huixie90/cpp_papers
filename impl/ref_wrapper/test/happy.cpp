#include "ref_wrapper.hpp"
#include "testutil.hpp"
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

// derived-base and implicit convertibles
struct B {};
struct D : B {};
struct C {
    operator B&() const;
};

static_assert(check<B&      , Ref<B>,       D &     >);
static_assert(check<B const&, Ref<B>,       D const&>);
static_assert(check<B const&, Ref<B const>, D const&>);


static_assert(check<B&      , Ref<D>,       B &     >);
static_assert(check<B const&, Ref<D>,       B const&>);
static_assert(check<B const&, Ref<D const>, B const&>);
// Interesting note: This last set works on VC++ thanks to our specialization.
// But on GCC, it works naturally since ternary operator is no longer ambiguous.
// Here:
#ifndef _MSC_VER
static_assert(std::same_as<B&, CondRes<Ref<D>, B&>>);
static_assert(std::same_as<B const&, CondRes<Ref<D>, B const &>>);
static_assert(std::same_as<B const&, CondRes<Ref<D const>, B const&>>);
#else
// but for some reason ternary operator on VC++ yields a base prvalue!
static_assert(std::same_as<B, CondRes<Ref<D>, B&>>);
static_assert(std::same_as<B const, CondRes<Ref<D>, B const &>>);
static_assert(std::same_as<B const, CondRes<Ref<D const>, B const&>>);
// fortunately, since this is not a reference type, common_reference rules fallthru
// basic_common_reference specialization, and we compute B& here.
#endif


// implicit conversions are two hops so shouldn't work:
static_assert( check_none<Ref<C>, B& >);
static_assert( !std::convertible_to<Ref<C>, B&>);


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


#ifdef CATCH2_DEFINE_MAIN
#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    // your setup ...

    int result = Catch::Session().run(argc, argv);

    // your clean-up...

    return result;
}
#endif

