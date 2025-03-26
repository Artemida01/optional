#pragma once

#include <compare>
#include <type_traits>

template <typename T>
class optional;

template <typename T, template <typename> typename Mold>
struct matches_mold : std::false_type {};

template <template <typename> typename Mold, typename T>
struct matches_mold<Mold<T>, Mold> : std::true_type {};

template <typename T>
struct is_optional : matches_mold<T, optional> {};

///**************************[Additional Information]**************************///

struct nullopt_t {
  enum class proxy {
    ALTERNATIVE
  };

  explicit constexpr nullopt_t(proxy) noexcept {}
};

struct in_place_t {
  explicit in_place_t() = default;
};

inline constexpr nullopt_t nullopt{nullopt_t::proxy::ALTERNATIVE};

inline constexpr in_place_t in_place{};

///**************************[Aliasing templates]**************************///

template <typename U, typename T>
using IsNotSelf = std::negation<std::is_same<std::remove_cvref_t<U>, optional<T>>>;

template <typename U>
using IsNotInPlace = std::negation<std::is_same<std::remove_cvref_t<U>, in_place_t>>;

template <typename T, typename U>
using IsNotBoolTrap =
    std::negation<std::conjunction<std::is_same<std::remove_cv_t<T>, bool>, is_optional<std::remove_cvref_t<U>>>>;

template <typename U, typename T>
using IsConstructible = std::is_constructible<T, U&&>;

template <typename U, typename T>
using EnableIfConstructible =
    std::enable_if_t<std::conjunction_v<IsConstructible<U, T>, IsNotSelf<U, T>, IsNotInPlace<U>, IsNotBoolTrap<U, T>>>;

template <typename U, typename T>
using IsNotScalarOrSame =
    std::disjunction<std::negation<std::is_scalar<T>>, std::negation<std::is_same<T, std::decay_t<U>>>>;

template <typename U, typename T>
using IsAssignable = std::conjunction<std::is_constructible<T, U>, std::is_assignable<T&, U>>;

template <typename U, typename T>
using EnableIfAssignable =
    std::enable_if_t<std::conjunction_v<IsNotSelf<U, T>, IsNotScalarOrSame<U, T>, IsAssignable<U, T>>>;

template <typename T>
using IsCopyable = std::conjunction<std::is_copy_constructible<T>, std::is_copy_assignable<T>>;

template <typename T>
using IsTriviallyCopyable = std::conjunction<
    std::is_trivially_copy_constructible<T>,
    std::is_trivially_copy_assignable<T>,
    std::is_trivially_destructible<T>>;

template <typename T>
using IsMoveable = std::conjunction<std::is_move_constructible<T>, std::is_move_assignable<T>>;

template <typename T>
using IsTriviallyMoveable = std::conjunction<
    std::is_trivially_move_constructible<T>,
    std::is_trivially_move_assignable<T>,
    std::is_trivially_destructible<T>>;

template <typename T>
using IsNothrowMovableAndSwappable =
    std::conjunction<std::is_nothrow_move_constructible<T>, std::is_nothrow_swappable<T>>;

template <typename T>
using IsMovableAndSwappable = std::conjunction<std::is_move_constructible<T>, std::is_swappable<T>>;
