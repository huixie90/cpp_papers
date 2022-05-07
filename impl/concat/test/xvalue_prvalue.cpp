
#include "concat.hpp"
#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <ranges>
#include <string>
#include <tuple>

#define TEST_POINT(x) TEST_CASE(x, "[xvalue_prvalue]")

struct Foo {
    int i;
    Foo(int ii)
        : i{ii} {}
    Foo(Foo&& other) noexcept
        : i{other.i} {
        other.i = -1;
    }
    Foo(const Foo&) = delete;
    Foo& operator=(Foo&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        i = other.i;
        other.i = -1;
        return *this;
    }
    Foo& operator=(const Foo&) = delete;
};

using namespace std::string_literals;

TEST_POINT("operator* : reference mixing xvalue with prvalue") {
    std::vector<std::string> v{"abc"s, "def"s};
    auto r1 = v | std::views::transform([](auto&& s) -> std::string&& { return std::move(s); });
    auto r2 =
        std::views::iota(0, 4) | std::views::transform([](int i) { return std::to_string(i); });

    auto cv = std::views::concat(r1, r2);
    auto it = cv.begin();
    CHECK(*it == "abc"s);
    *it;
    CHECK(*it == ""s); // SURPRISE! should've been same "abc"

    it += 2;
    *it;
    CHECK(*it == "0"s);
}

TEST_POINT("operator* : reference mixing lvalue-ref with prvalue") {
    std::vector<std::string> v{"abc"s, "def"s};
    auto r2 =
        std::views::iota(0, 4) | std::views::transform([](int i) { return std::to_string(i); });

    auto cv = std::views::concat(v, r2);
    auto it = cv.begin();
    *it;
    CHECK(*it == "abc"s);

    it += 2;
    *it;
    CHECK(*it == "0"s);
}

TEST_POINT("operator* : reference_wrapper") {
    std::string s{"abc"};
    std::vector<std::string> v{"def"s, "ghi"s};

    auto cv = std::views::concat(std::ranges::single_view{std::ref(s)}, v);
    auto it = cv.begin();
    *it;
    CHECK(*it == "abc"s);

    ++it;
    *it;
    CHECK(*it == "def"s);
}

TEST_POINT("operator* : reference_wrapper with prvalue") {
    std::string s{"abc"};
    auto r2 =
        std::views::iota(0, 4) | std::views::transform([](int i) { return std::to_string(i); });

    auto cv = std::views::concat(std::ranges::single_view{std::ref(s)}, r2);
    auto it = cv.begin();
    *it;
    CHECK(*it == "abc"s);

    ++it;
    *it;
    CHECK(*it == "0"s);
}

TEST_POINT("operator* : move only") {
    std::vector<Foo> v{};
    v.emplace_back(5);
    auto r1 = v | std::views::transform([](auto&& f) -> Foo&& { return std::move(f); });
    auto r2 = std::views::iota(0, 4) | std::views::transform([](int i) { return Foo(i); });

    auto cv = std::views::concat(r1, r2);
    auto it = cv.begin();
    CHECK((*it).i == 5);
    CHECK((*it).i == -1); // SURPRISE! previous *it called move constructor

    ++it;
    *it;
    CHECK((*it).i == 0);
}

TEST_POINT(
    "iter_move : r1: reference:lvalue rvalue_reference:xvalue.\nr2: reference and "
    "rvalue-reference: prvalue ") {
    std::vector<std::string> v{"abc"s, "def"s};
    auto r2 =
        std::views::iota(0, 4) | std::views::transform([](int i) { return std::to_string(i); });

    auto cv = std::views::concat(v, r2);
    auto it = cv.begin();

    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value = std::ranges::iter_move(it);
    CHECK(value.empty());
    //    ^^^ SURPRISE! value should have been "abc", but unused iter_move destroyed it.

    it += 2;
    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value2 = std::ranges::iter_move(it);
    CHECK(value2 == "0"s);
}

TEST_POINT("iter_move : reference_wrapper") {
    std::string s{"abc"};
    std::vector<std::string> v{"def"s, "ghi"s};

    auto cv = std::views::concat(std::ranges::single_view{std::ref(s)}, v);
    auto it = cv.begin();

    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value = std::ranges::iter_move(it);
    CHECK(value == "abc"s);

    ++it;
    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value2 = std::ranges::iter_move(it);
    CHECK(value2 == ""s); // SURPRISE! should be "def", but unused iter_move destroyed it.
}

TEST_POINT("iter_move : reference_wrapper with prvalue") {
    std::string s{"abc"};
    auto r2 =
        std::views::iota(0, 4) | std::views::transform([](int i) { return std::to_string(i); });

    auto cv = std::views::concat(std::ranges::single_view{std::ref(s)}, r2);
    auto it = cv.begin();

    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value = std::ranges::iter_move(it);
    CHECK(value == "abc"s);

    ++it;
    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value2 = std::ranges::iter_move(it);
    CHECK(value2 == "0"s); // fails at the moment
}

TEST_POINT("iter_move : move only") {
    std::vector<Foo> v{};
    v.emplace_back(5);
    auto r1 = v | std::views::transform([](auto&& f) -> Foo&& { return std::move(f); });
    auto r2 = std::views::iota(0, 4) | std::views::transform([](int i) { return Foo(i); });

    auto cv = std::views::concat(r1, r2);
    auto it = cv.begin();

    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value = std::ranges::iter_move(it);
    CHECK(value.i == -1);
    //    ^^^ SURPRISE! unused iter_move() already called move constructor.

    ++it;
    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value2 = std::ranges::iter_move(it);
    CHECK(value2.i == 0);
}
