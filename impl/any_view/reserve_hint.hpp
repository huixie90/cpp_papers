#pragma once

#include <ranges>

namespace std::ranges {

namespace __reserve_hint {

struct __fn {
  template <class Rng>
    requires std::ranges::sized_range<Rng>
  static constexpr decltype(auto) operator()(Rng&& rng) noexcept(
      noexcept(std::ranges::size(std::forward<Rng>(rng)))) {
    return std::ranges::size(std::forward<Rng>(rng));
  }

  template <class Rng>
  static constexpr auto operator()(Rng&& rng) noexcept(
      noexcept(std::forward<Rng>(rng).reserve_hint()))
      -> decltype(std::forward<Rng>(rng).reserve_hint()) {
    return std::forward<Rng>(rng).reserve_hint();
  }
};
};  // namespace __reserve_hint

inline constexpr __reserve_hint::__fn reserve_hint{};

template <class T>
concept approximately_sized_range =
    range<T> && requires(T& t) { ranges::reserve_hint(t); };

}  // namespace std::ranges
