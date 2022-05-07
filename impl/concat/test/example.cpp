#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <ranges>
#include <functional>

#define TEST_POINT(x) TEST_CASE(x, "[example]")



TEST_POINT("example 1: vector, array, and scalar") {
    std::vector<int> v1{1, 2, 3}, v2{4, 5}, v3{};
    std::array a{6, 7, 8};

    auto s = std::ranges::single_view(9);
    auto cv = std::views::concat(v1, v2, v3, a, s);

    std::vector expected{1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (auto i = 0; i != 9; ++i) {
        REQUIRE(cv[i] == expected[i]);
    }

    STATIC_CHECK(std::same_as<std::ranges::range_reference_t<decltype(cv)>, int&>);
}

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

struct Bar {
    Foo foo;
    const Foo& getFoo() const { return foo; };
};

struct MyClass {
    std::vector<Foo> foos_;
    Bar bar_;

    auto getFoos_before() const {
        std::vector<std::reference_wrapper<Foo const>> fooRefs;
        fooRefs.reserve(foos_.size() + 1);

        for (auto const& f : foos_) {
            fooRefs.emplace_back(f);
        }
        fooRefs.emplace_back(bar_.getFoo());
        return fooRefs;
    }

    auto getFoos() const {
        return std::views::concat(
            foos_, std::views::single(std::cref(bar_)) | std::views::transform(&Bar::getFoo));
    }
};
} // namespace

TEST_POINT("example 2: segmented data") {
    MyClass c{{}, Bar{Foo{4}}};
    c.foos_.reserve(3);
    c.foos_.emplace_back(1);
    c.foos_.emplace_back(2);
    c.foos_.emplace_back(3);

    std::vector expected{1, 2, 3, 4};

    for (int i = 0; const Foo& foo : c.getFoos_before()) {
        CHECK(foo.i == expected[i]);
        ++i;
    }

    for (int i = 0; const Foo& foo : c.getFoos()) {
        CHECK(foo.i == expected[i]);
        ++i;
    }
}