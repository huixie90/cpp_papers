#include "ref_wrapper.hpp"
#include <catch2/catch_test_macros.hpp>
#include "testutil.hpp"


#define TEST_POINT(x) TEST_CASE(x, "[sad]")

using namespace std;



static_assert(!HasType<Ternary<Ref<int> const&, int&>>); // ok, ternary is still ambiguous
static_assert(
    std::convertible_to<Ref<int> const&, int&>);         // Ref<int> const& CAN be converted to int&
static_assert(check<int const&, Ref<int> const&, int&>); // BUT we compute common ref as const int &
#ifndef _MSC_VER
static_assert(
    std::same_as<int const&, CommonRef<Ref<int> const&, int&>>); // BECAUSE of this first rule of
                                                                 // common_reference
#endif



// T& and reference_wrapper<T> are substitutable with identical common_reference
template <typename T>
struct Test {
    using R1 = common_reference_t<T&, T&>;
    using R2 = common_reference_t<T&, T const&>;
    using R3 = common_reference_t<T&, T&&>;
    using R4 = common_reference_t<T&, T const&&>;
    using R5 = common_reference_t<T&, T>;

    using U = reference_wrapper<T>;

    static_assert(same_as<R1, common_reference_t<U, T&>>);
    static_assert(same_as<R2, common_reference_t<U, T const&>>);
    static_assert(same_as<R3, common_reference_t<U, T&&>>);
    static_assert(same_as<R4, common_reference_t<U, T const&&>>);
    static_assert(same_as<R5, common_reference_t<U, T>>);

    // commute:
    static_assert(same_as<R1, common_reference_t<T&, U>>);
    static_assert(same_as<R2, common_reference_t<T const&, U>>);
    static_assert(same_as<R3, common_reference_t<T&&, U>>);
    static_assert(same_as<R4, common_reference_t<T const&&, U>>);
    static_assert(same_as<R5, common_reference_t<T, U>>);

    // reference qualification of reference_wrapper is irrelevant (except KINKs
    // we may need to resolve - see below)
    static_assert(same_as<R1, common_reference_t<U&, T&>>);
    static_assert(same_as<R1, common_reference_t<U, T&>>);
    // !! KINKS !!
    static_assert(same_as<R2, common_reference_t<U const&, T&>>); // NOT R1 which is non-const &
    static_assert(same_as<R2, common_reference_t<U&&, T&>>);
    static_assert(same_as<R2, common_reference_t<U const&&, T&>>);
};

// Instantiate above checks:
template struct Test<int>;
template struct Test<reference_wrapper<int>>;

// !! KINKS !!
//  unfortunately,  ?: is not ambigous with const & and && reference_wrapper so
//  the specialiation is never consulted:

// per https://eel.is/c++draft/meta.trans.other#2.5
// common_reference< X&, Y const &> has  const& for both sides of ternary-check:
using Kink =
    decltype(false ? std::declval<int const&>() : std::declval<reference_wrapper<int> const&>());
// the ?: is no longer ambigous and resolves to:
static_assert(same_as<Kink, int const&>);
static_assert(same_as<Kink, common_reference_t<int&, reference_wrapper<int> const&>>);

// per https://eel.is/c++draft/meta.trans.other#2.7
// common_reference<X&, Y&&> checks for <X&, Y const &> again, same as above:
static_assert(same_as<Kink, common_reference_t<int&, reference_wrapper<int>&&>>);


/////////////////////////////////////////////////////
/// Unrelated cases involving reference_wrapper:  ///
/////////////////////////////////////////////////////

// reference_wrapper as both args is unaffected. unrelated to the proposal,
// subject to simple first rule of
static_assert(same_as<common_reference_t<reference_wrapper<int>&, reference_wrapper<int>&>,
                      reference_wrapper<int>&>);

// double wrap is unaffected. unrelated to the proposal:
template <class T>
concept CommonRefWithNestedRefWrapExists =
    requires(T) { typename common_reference_t<reference_wrapper<reference_wrapper<T>>, T&>; };
static_assert(!CommonRefWithNestedRefWrapExists<int>);


static_assert(
    same_as<common_reference_t<reference_wrapper<int>&, reference_wrapper<reference_wrapper<int>>>,
            reference_wrapper<int>&>);
