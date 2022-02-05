#pragma once
#include <iterator>
#include <ranges>

//  +-------------------+
//  |  Cpp17OutputIter  |
//  +-------------------+

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


//  +------------------+
//  |  Cpp17InputIter  |
//  +------------------+

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

    // must pin iterator category, otherwise it models std::forward_iterator<It>
    using iterator_category = std::input_iterator_tag;
    // using iterator_concept = std::input_iterator_tag; // also works
};



//  +--------------------+
//  |  Cpp17ForwardIter  |
//  +--------------------+

struct Cpp17ForwardIter {
    using It = int*;
    It it;

    int const& operator*() const { return *it; }

    Cpp17ForwardIter& operator++() {
        ++it;
        return *this;
    }
    Cpp17ForwardIter operator++(int) { return {it++}; }

    bool operator==(Cpp17ForwardIter other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};


//  +---------------------------+
//  |  Cpp17ForwardMutableIter  |
//  +---------------------------+
struct Cpp17ForwardMutableIter {
    using It = int*;
    It it;

    int& operator*() const { return *it; }

    Cpp17ForwardMutableIter& operator++() {
        ++it;
        return *this;
    }
    Cpp17ForwardMutableIter operator++(int) { return {it++}; }

    bool operator==(Cpp17ForwardMutableIter other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};


//  +--------------------------+
//  |  Cpp17BidirectionalIter  |
//  +--------------------------+
struct Cpp17BidirectionalIter {
    using It = int*;
    It it;

    int& operator*() const { return *it; }

    Cpp17BidirectionalIter& operator++() {
        ++it;
        return *this;
    }
    Cpp17BidirectionalIter operator++(int) { return {it++}; }

    Cpp17BidirectionalIter& operator--() {
        --it;
        return *this;
    }
    Cpp17BidirectionalIter operator--(int) { return {it--}; }


    bool operator==(Cpp17BidirectionalIter other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};



//  +------------------+
//  |  Cpp20InputIter  |
//  +------------------+

// void postfix, rvalue return
struct Cpp20InputIter {
    using It = int*;
    It it;

    int operator*() const { return *it; }

    Cpp20InputIter& operator++() {
        ++it;
        return *this;
    }
    void operator++(int) { it++; }

    bool operator==(Cpp20InputIter other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};


// regular postfix, rvalue return, BUT no comparison
// - this is a c++20 input_iterator since it has sentinel comparison
// - not a c++20 output_iterator since it has rvalue return
// - and only a c++17 output_iterator, since it has no self comparison
struct Cpp20InputCpp17OutputIter {
    using It = int*;
    struct Sentinel {
        It it;
    };
    It it;

    int operator*() const { return *it; }

    Cpp20InputCpp17OutputIter& operator++() {
        ++it;
        return *this;
    }
    Cpp20InputCpp17OutputIter operator++(int) { return {it++}; }

    bool operator==(Sentinel other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};



//  +--------------------+
//  |  Cpp20ForwardIter  |
//  +--------------------+

struct Cpp20ForwardIter {
    using It = int*;
    It it;

    int operator*() const { return *it; }

    Cpp20ForwardIter& operator++() {
        ++it;
        return *this;
    }
    Cpp20ForwardIter operator++(int) { return {it++}; }

    bool operator==(Cpp20ForwardIter other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};



struct Cpp20ForwardIterWithTraits {
    using It = int*;
    It it;

    int operator*() const { return *it; }

    Cpp20ForwardIterWithTraits& operator++() {
        ++it;
        return *this;
    }
    Cpp20ForwardIterWithTraits operator++(int) { return {it++}; }

    bool operator==(Cpp20ForwardIterWithTraits other) const { return it == other.it; }

    using value_type = int;                 // to model indirectly_readable_traits
    using difference_type = std::ptrdiff_t; // to model incrementable_traits
};


namespace std {
// without this specialization, it works as expected: C++17 input, and C++20 forward iterator
//
// with this specialization (which isn't necessary here, but in case it was necessary for other
// implementation detail, such as the counting_iterator case which is disabling operator->) it would
// end up being in the same category as what it inherits (input_iterator_tag here). But we *really*
// want it to be C++20 forward iterator since it really is.

template <>
struct iterator_traits<Cpp20ForwardIterWithTraits> : iterator_traits<Cpp20ForwardIter> {
    using iterator_concept = forward_iterator_tag;
    // if this wasn't defined
    // std::foward_iterator<Cpp20ForwardIterWithTraits> would fail
    // we can not define iterator_category because it should stay as input_iterator_tag for C++17
    // (rvalue reference)
};
} // namespace std



// make a subrange out of above from a given int* range
template <class It, std::ranges::contiguous_range R>
auto getRangeOf(R&& r) {
    return std::ranges::subrange(It{&*std::ranges::begin(r)}, It{&*std::ranges::end(r)});
}
