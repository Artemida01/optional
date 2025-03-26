#pragma once

#include "comn_inf.h"

#include <compare>
#include <utility>

template <typename T>
class optional : protected MoveAssignCondition<T> {
public:
  using value_type = T;
  using base = MoveAssignCondition<T>;

public:
  constexpr optional() noexcept = default;

  constexpr optional(nullopt_t) noexcept {}

  constexpr optional(const optional&) = default;
  constexpr optional(optional&&) = default;

  constexpr optional& operator=(const optional&) = default;
  constexpr optional& operator=(optional&&) = default;

  template <typename U = T, typename = EnableIfConstructible<U, T>>
  constexpr explicit(!std::is_convertible_v<U&&, T>) optional(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
      : optional(in_place, std::forward<U>(value)) {}

  template <typename... Args>
  explicit constexpr optional(in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    emplace(std::forward<Args>(args)...);
  }

  constexpr optional& operator=(nullopt_t) noexcept(std::is_nothrow_destructible_v<T>) {
    if (has_value()) {
      reset();
    }
    return *this;
  }

  template <typename U = T, typename = EnableIfAssignable<U, T>>
  constexpr optional& operator=(U&& val
  ) noexcept(std::is_nothrow_assignable_v<T&, U> && std::is_nothrow_constructible_v<T, U>) {
    if (has_value()) {
      **this = std::forward<U>(val);
    } else {
      emplace(std::forward<U>(val));
    }
    return *this;
  }

  constexpr void swap(optional& other) noexcept {
    using std::swap;
    if (has_value()) {
      if (other.has_value()) {
        swap(this->_is_ready, other._is_ready);
        swap(*other, **this);
      } else {
        other.emplace(std::move(**this));
        reset();
      }
    } else if (other.has_value()) {
      other.swap(*this);
    }
  }

  constexpr bool has_value() const noexcept {
    return base::has_value();
  }

  constexpr explicit operator bool() const noexcept {
    return has_value();
  }

  constexpr T* operator->() noexcept {
    return std::addressof(optional_impl<T>::_element_block.value);
  }

  constexpr const T* operator->() const noexcept {
    return std::addressof(optional_impl<T>::_element_block.value);
  }

  constexpr T& operator*() & noexcept {
    return optional_impl<T>::_element_block.value;
  }

  constexpr const T& operator*() const& noexcept {
    return optional_impl<T>::_element_block.value;
  }

  constexpr T&& operator*() && noexcept {
    return std::move(optional_impl<T>::_element_block.value);
  }

  constexpr const T&& operator*() const&& noexcept {
    return std::move(optional_impl<T>::_element_block.value);
  }

  template <typename... Args>
  constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
    reset();
    optional_impl<T>::construct(std::forward<Args>(args)...);
    return **this;
  }

  constexpr void reset() noexcept {
    base::reset();
  }
};

template <typename T>
optional(T) -> optional<T>;

namespace {

template <typename T, typename Operator>
constexpr auto compare_optionals(const optional<T>& lhs, const optional<T>& rhs, Operator op) noexcept(
    noexcept(op(*lhs, *rhs)) && noexcept(op(lhs.has_value(), rhs.has_value()))
) {
  const bool lhs_has = lhs.has_value();
  const bool rhs_has = rhs.has_value();

  if (lhs_has && rhs_has) {
    return op(*lhs, *rhs);
  }
  return op(lhs_has, rhs_has);
}

} // namespace

template <class T>
constexpr std::compare_three_way_result_t<T> operator<=>(const optional<T>& lhs, const optional<T>& rhs) noexcept(
    noexcept(std::declval<T>() <=> std::declval<T>())
) {
  return compare_optionals(lhs, rhs, std::compare_three_way());
}

template <typename T>
constexpr bool
operator==(const optional<T>& lhs, const optional<T>& rhs) noexcept(noexcept(std::declval<T>() == std::declval<T>())) {
  return compare_optionals(lhs, rhs, std::equal_to<>());
}

template <typename T>
constexpr bool
operator!=(const optional<T>& lhs, const optional<T>& rhs) noexcept(noexcept(std::declval<T>() != std::declval<T>())) {
  return compare_optionals(lhs, rhs, std::not_equal_to<>());
}

template <typename T>
constexpr bool
operator<(const optional<T>& lhs, const optional<T>& rhs) noexcept(noexcept(std::declval<T>() < std::declval<T>())) {
  return compare_optionals(lhs, rhs, std::less<>());
}

template <typename T>
constexpr bool
operator<=(const optional<T>& lhs, const optional<T>& rhs) noexcept(noexcept(std::declval<T>() <= std::declval<T>())) {
  return compare_optionals(lhs, rhs, std::less_equal<>());
}

template <typename T>
constexpr bool
operator>(const optional<T>& lhs, const optional<T>& rhs) noexcept(noexcept(std::declval<T>() > std::declval<T>())) {
  return compare_optionals(lhs, rhs, std::greater<>());
}

template <typename T>
constexpr bool
operator>=(const optional<T>& lhs, const optional<T>& rhs) noexcept(noexcept(std::declval<T>() >= std::declval<T>())) {
  return compare_optionals(lhs, rhs, std::greater_equal<>());
}

template <typename T>
std::enable_if<!IsMovableAndSwappable<T>::value>::type swap(optional<T>&, optional<T>&) = delete;

template <typename T>
inline constexpr std::enable_if<IsMovableAndSwappable<T>::value>::type
swap(optional<T>& lhs, optional<T>& rhs) noexcept(IsNothrowMovableAndSwappable<T>::value) {
  lhs.swap(rhs);
}
