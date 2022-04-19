
#include "concat.hpp"
#include "range_fixtures.hpp"
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
    Foo(Foo&& other)
        : i{other.i} {
        other.i = -1;
    }
    Foo(const Foo&) = delete;
    Foo& operator=(Foo&& other) {
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
    *it;
    // CHECK(*it == "abc"s); // does not compile
    CHECK(std::string(*it) == "abc"s);

    it += 2;
    *it;
    // CHECK(*it == "0"s);// does not compile
    CHECK(std::string(*it) == "0"s);
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

/*

This no longer works because the following in indirectly_readable fails
std::common_reference_with<std::iter_reference_t<In>&&, std::iter_value_t<In>&>

It used to work because
common_reference_t<MoveOnly&&, MoveOnly&> is const MoveOnly& and both operands can be converted to it.

But
common_reference_t<Proxy&&, MoveOnly&> is MoveOnly and the second operator cannot be converted to it

TEST_POINT("operator* : move only") {
    std::vector<Foo> v{};
    v.emplace_back(5);
    auto r1 = v | std::views::transform([](auto&& f) -> Foo&& { return std::move(f); });
    auto r2 = std::views::iota(0, 4) | std::views::transform([](int i) { return Foo(i); });

    auto cv = std::views::concat(r1, r2);
    auto it = cv.begin();
    *it;
    // CHECK((*it).i == 5); // does not compile
    CHECK(Foo{(*it)}.i == 5);

    ++it;
    *it;
    CHECK(Foo{(*it)}.i == 0);
}
*/

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
    CHECK(value == "abc"s); // pass with proxies

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
    CHECK(value2 == "def"s); // pass with proxies
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
    CHECK(value2 == "0"s); // pass with proxies
}
/*

This no longer works because the following in indirectly_readable fails
std::common_reference_with<std::iter_reference_t<In>&&, std::iter_value_t<In>&>

It used to work because
common_reference_t<MoveOnly&&, MoveOnly&> is const MoveOnly& and both operands can be converted to it.

But
common_reference_t<Proxy&&, MoveOnly&> is MoveOnly and the second operator cannot be converted to it

TEST_POINT("iter_move : move only") {
    std::vector<Foo> v{};
    v.emplace_back(5);
    auto r1 = v | std::views::transform([](auto&& f) -> Foo&& { return std::move(f); });
    auto r2 = std::views::iota(0, 4) | std::views::transform([](int i) { return Foo(i); });

    auto cv = std::views::concat(r1, r2);
    auto it = cv.begin();

    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value = std::ranges::iter_move(it);
    CHECK(value.i == 5); // pass with proxies

    ++it;
    std::ignore = std::ranges::iter_move(it); // in libcxx iter_move is marked nodiscard
    std::ranges::range_value_t<decltype(cv)> value2 = std::ranges::iter_move(it);
    CHECK(value2.i == 0);
}
*/
template <typename... R>
concept concat_able = requires(R&&... r) { std::ranges::concat_view{(R &&) r...}; };


struct InputRangePRValue : std::ranges::view_base {
    Cpp17InputIter begin() const;
    Cpp17InputIter end() const;
};
static_assert(std::ranges::input_range<InputRangePRValue>);
static_assert(!std::ranges::forward_range<InputRangePRValue>);
static_assert(std::same_as<int, std::ranges::range_reference_t<InputRangePRValue>>);
static_assert(std::same_as<int, std::ranges::range_rvalue_reference_t<InputRangePRValue>>);


TEST_POINT("deref proxy input range not concatable") {
    std::vector<int> v{1, 2, 3};
    auto xvalue = v | std::views::transform([](auto&& i) -> int&& { return std::move(i); });
    using XValueRange = decltype(xvalue);
    static_assert(concat_able<InputRangePRValue, InputRangePRValue>);
    static_assert(concat_able<XValueRange, XValueRange>);
    static_assert(!concat_able<InputRangePRValue, XValueRange>);
}


struct cpp20_input_iterator {
    using It = int*;
    It it;

    decltype(auto) operator*() const { return *it; }

    cpp20_input_iterator& operator++() {
        ++it;
        return *this;
    }
    void operator++(int) { it++; }

    friend bool operator==(cpp20_input_iterator const&, cpp20_input_iterator const&) = default;

    using value_type = std::iter_value_t<It>;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;
};

struct InputRangeRef{
    cpp20_input_iterator begin() const;
    cpp20_input_iterator end() const;
};
static_assert(std::ranges::input_range<InputRangeRef>);
static_assert(!std::ranges::forward_range<InputRangeRef>);
static_assert(std::same_as<int&, std::ranges::range_reference_t<InputRangeRef>>);
static_assert(std::same_as<int&&, std::ranges::range_rvalue_reference_t<InputRangeRef>>);

TEST_POINT("iter_move proxy input range not concatable") {
    static_assert(concat_able<InputRangeRef, InputRangeRef>);
    static_assert(!concat_able<InputRangeRef, InputRangePRValue>);
}

