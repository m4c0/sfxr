#pragma once

#include <algorithm>
#include <optional>

template<class Tp>
static constexpr std::optional<Tp> cond(bool yes, Tp val) {
  if (yes) return { val };
  return {};
}
template<class Tp>
static constexpr auto opt_chain(std::optional<Tp> a, std::optional<Tp> b) {
  if (a) return a;
  return b;
}
template<class Tp, auto N>
static constexpr std::array<Tp, N> concat(const std::array<Tp, N> & a) {
  return a;
}
template<class Tp, auto N, auto... Ns>
static constexpr std::array<Tp, N + (Ns + ...)> concat(const std::array<Tp, N> & a, const std::array<Tp, Ns>... o) {
  const auto b = concat(o...);

  std::array<Tp, N + (Ns + ...)> result {};
  std::copy(a.cbegin(), a.cend(), result.begin());
  std::copy(b.cbegin(), b.cend(), result.begin() + N);
  return result;
}

class count_iter {
  int idx {};

public:
  [[nodiscard]] constexpr auto operator*() const noexcept {
    return idx;
  }
  [[nodiscard]] constexpr count_iter & operator++() noexcept {
    idx++;
    return *this;
  }
  [[nodiscard]] constexpr count_iter operator++(int) noexcept { // NOLINT
    count_iter res = *this;
    idx++;
    return res;
  }
};
template<class To, class From, auto N, class Fn>
static constexpr std::array<To, N> convert(const std::array<From, N> & from, Fn && fn) {
  std::array<To, N> to {};
  std::transform(from.begin(), from.end(), to.begin(), fn);
  return to;
}
template<class To, class From, auto N, class Fn>
static constexpr std::array<To, N> convert_indexed(const std::array<From, N> & from, Fn && fn) {
  std::array<To, N> to {};
  std::transform(from.begin(), from.end(), count_iter {}, to.begin(), fn);
  return to;
}

template<class Arr, typename T, T... idxs>
static constexpr auto sum_all(const Arr & arr, std::integer_sequence<T, idxs...> /*seq*/) {
  return (arr[idxs] + ...);
}
template<class Arr>
static constexpr auto sum_all(const Arr & arr) {
  return sum_all(arr, std::make_integer_sequence<int, Arr().size()>());
}

template<class Arr, typename T, T... idxs>
static constexpr auto concat_all(const Arr & arr, std::integer_sequence<T, idxs...> /*seq*/) {
  return concat(arr[idxs]...);
}
template<class Arr>
static constexpr auto concat_all(const Arr & arr) {
  return concat_all(arr, std::make_integer_sequence<int, Arr().size()>());
}

template<typename A, typename B, auto N, typename Fn>
static auto zip(const std::array<A, N> & a, const std::array<B, N> & b, Fn && fn) {
  std::array<decltype(fn(A {}, B {})), N> res {};
  std::transform(a.begin(), a.end(), b.begin(), res.begin(), fn);
  return res;
}
