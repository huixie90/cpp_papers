#include "ref_wrapper.hpp"

#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <ranges>
#include <functional>

#define TEST_POINT(x) TEST_CASE(x, "[refwrap]")



namespace {

// Foo's copy/move creates observable side-effects to be able detect if called unexpectedly
struct Foo {
    Foo(int i)
        : i(i) {}
    Foo(Foo const& f)
        : i(f.i + 100) {}
    Foo& operator=(Foo const& f) {
        i = f.i + 200;
        return *this;
    }
    Foo(Foo&& f) noexcept {
        i = f.i + 1000;
        f.i += -1000;
    }
    Foo& operator=(Foo&& f) noexcept {
        i = f.i + 2000;
        f.i -= -2000;
        return *this;
    }
    ~Foo() { i = 42; }
    int i;
};

struct MyClass {
    std::vector<Foo> foos_;
    Foo foo;

    auto getFoos() { return std::views::concat(foos_, std::views::single(std::ref(foo))); }
    auto getFoosConst() const { // << just the method is const. otherwise the same.
        return std::views::concat(foos_, std::views::single(std::ref(foo)));
    }
};
} // namespace

TEST_POINT("example 2: segmented data") {
    MyClass c{{}, Foo{4}};
    c.foos_.reserve(3);
    c.foos_.emplace_back(1);
    c.foos_.emplace_back(2);
    c.foos_.emplace_back(3);

    std::vector expected{1, 2, 3, 4};

    auto concatRange = c.getFoos();
    static_assert(std::same_as<Foo&, std::ranges::range_reference_t<decltype(concatRange)>>);

    auto concatRangeC = c.getFoosConst();
    static_assert(std::same_as<Foo const&, std::ranges::range_reference_t<decltype(concatRangeC)>>);
}
