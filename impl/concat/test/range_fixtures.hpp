#pragma once
#include <cstddef>
#include <xutility>

struct Cpp17OutputIter {
    using It = int*;
    It it;
    struct proxy {
        It it;
        void operator=(int i) const { *it = i; }
    };
    proxy operator*() { return {it}; }

    Cpp17OutputIter& operator++() {
        ++it;
        return *this;
    }

    Cpp17OutputIter operator++(int) { return {it++}; }

    bool operator==(Cpp17OutputIter other) const { return it == other.it; }
    using difference_type = std::ptrdiff_t; // for incrementable_traits
};

struct Cpp17InputIter {
    using It = int*;
    It it;

    int operator*() const { return *it; }

    Cpp17InputIter& operator++() {
        ++it;
        return *this;
    }
    Cpp17InputIter operator++(int) { return {it++}; }

    bool operator==(Cpp17InputIter other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
    // using pointer = int*;
    using reference = int&;

    // must pin iterator category, otherwise it models std::forward_iterator<It>
    using iterator_category = std::input_iterator_tag;
    //using iterator_concept = std::input_iterator_tag; // also works
};
