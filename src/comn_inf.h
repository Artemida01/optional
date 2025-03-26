#pragma once

#include "type_traits_extensions.h"

#include <memory>

///**************************[Specification]**************************///

enum class TInfo {
  DELETED,
  TRIVIAL,
  BASIC,
};

template <bool TypeCondition, bool TypeTrivialCondition>
constexpr TInfo by_type() {
  if constexpr (!TypeCondition) {
    return TInfo::DELETED;
  }
  if constexpr (TypeTrivialCondition) {
    return TInfo::TRIVIAL;
  }
  return TInfo::BASIC;
}

//--------------------------------------------- DEFAULT BASES

template <
    typename T,
    bool Destroy = !std::is_trivially_destructible_v<T>,
    bool Construct = !std::is_trivially_default_constructible_v<T>>
union default_base {
  T value;
};

template <typename T>
union default_base<T, true, false> {
  T value;

  constexpr ~default_base() {}
};

template <typename T>
union default_base<T, false, true> {
public:
  T value;

  constexpr default_base() noexcept {}
};

template <typename T>
union default_base<T, true, true> {
  T value;

  constexpr ~default_base() {}

  constexpr default_base() noexcept {}
};

//--------------------------------------------- BASES

template <typename T>
struct optional_impl {
public:
  template <typename U>
  friend class optional;

public:
  constexpr explicit optional_impl() = default;

protected:
  constexpr T& operator*() & noexcept {
    return optional_impl<T>::_element_block.value;
  }

  constexpr const T& operator*() const& noexcept {
    return optional_impl<T>::_element_block.value;
  }

  template <typename... Args>
  constexpr void construct(Args&&... args) {
    std::construct_at(std::addressof(_element_block.value), std::forward<Args>(args)...);
    _is_ready = true;
  }

  constexpr void reset() {
    if (_is_ready) {
      std::destroy_at(std::addressof(_element_block.value));
      _is_ready = false;
    }
  }

  constexpr bool has_value() const noexcept {
    return _is_ready;
  }

private:
  default_base<T> _element_block;
  bool _is_ready{false};
};

///**************************[RULE OF 3]**************************///

//--------------------------------------------- DESTROY

template <typename T, bool TriviallyDestructible = std::is_trivially_destructible_v<T>>
struct DestroyCondition : optional_impl<T> {
  using optional_impl<T>::optional_impl;
};

template <typename T>
struct DestroyCondition<T, false> : optional_impl<T> {
  using optional_impl<T>::optional_impl;

  constexpr DestroyCondition() = default;
  constexpr DestroyCondition(const DestroyCondition&) = default;
  constexpr DestroyCondition(DestroyCondition&&) = default;
  constexpr DestroyCondition& operator=(const DestroyCondition&) = default;
  constexpr DestroyCondition& operator=(DestroyCondition&&) = default;

  constexpr ~DestroyCondition() noexcept(std::is_nothrow_destructible_v<T>) {
    optional_impl<T>::reset();
  }
};

//--------------------------------------------- COPY CTOR

template <
    typename T,
    TInfo COND = by_type<std::is_copy_constructible_v<T>, std::is_trivially_copy_constructible_v<T>>()>
struct CopyConstructCond : DestroyCondition<T> {
  using DestroyCondition<T>::DestroyCondition;
};

template <typename T>
struct CopyConstructCond<T, TInfo::DELETED> : DestroyCondition<T> {
  using DestroyCondition<T>::DestroyCondition;

  constexpr CopyConstructCond() = default;
  constexpr CopyConstructCond(const CopyConstructCond& other) = delete;
  constexpr CopyConstructCond(CopyConstructCond&&) = default;
  constexpr CopyConstructCond& operator=(const CopyConstructCond&) = default;
  constexpr CopyConstructCond& operator=(CopyConstructCond&&) = default;
};

template <typename T>
struct CopyConstructCond<T, TInfo::BASIC> : DestroyCondition<T> {
  using DestroyCondition<T>::DestroyCondition;

  constexpr CopyConstructCond() = default;
  constexpr CopyConstructCond(CopyConstructCond&&) = default;
  constexpr CopyConstructCond& operator=(const CopyConstructCond&) = default;
  constexpr CopyConstructCond& operator=(CopyConstructCond&&) = default;

  constexpr CopyConstructCond(const CopyConstructCond& rhs) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    if (rhs.has_value()) {
      optional_impl<T>::construct(*rhs);
    }
  }
};

//--------------------------------------------- COPY ASSIGN

template <typename T, TInfo COND = by_type<IsCopyable<T>::value, IsTriviallyCopyable<T>::value>()>
struct CopyAssignCondition : CopyConstructCond<T> {
  using CopyConstructCond<T>::CopyConstructCond;
};

template <typename T>
struct CopyAssignCondition<T, TInfo::BASIC> : CopyConstructCond<T> {
  using CopyConstructCond<T>::CopyConstructCond;

  constexpr CopyAssignCondition() = default;
  constexpr CopyAssignCondition(const CopyAssignCondition&) = default;
  constexpr CopyAssignCondition(CopyAssignCondition&&) = default;
  constexpr CopyAssignCondition& operator=(CopyAssignCondition&&) = default;

  constexpr CopyAssignCondition& operator=(const CopyAssignCondition& other
  ) noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>) {
    if (other.has_value()) {
      if (optional_impl<T>::has_value()) {
        **this = *other;
      } else {
        optional_impl<T>::construct(*other);
      }
    } else {
      optional_impl<T>::reset();
    }
    return *this;
  }
};

template <typename T>
struct CopyAssignCondition<T, TInfo::DELETED> : CopyConstructCond<T> {
  using CopyConstructCond<T>::CopyConstructCond;

  constexpr CopyAssignCondition& operator=(const CopyAssignCondition&) = delete;

  constexpr CopyAssignCondition() = default;
  constexpr CopyAssignCondition(const CopyAssignCondition&) = default;
  constexpr CopyAssignCondition(CopyAssignCondition&&) = default;
  constexpr CopyAssignCondition& operator=(CopyAssignCondition&&) = default;
};

///**************************[RULE OF 5]**************************///

//--------------------------------------------- MOVE CTOR

template <
    typename T,
    TInfo COND = by_type<std::is_move_constructible_v<T>, std::is_trivially_move_constructible_v<T>>()>
struct MoveCondition : CopyAssignCondition<T> {
  using CopyAssignCondition<T>::CopyAssignCondition;
};

template <typename T>
struct MoveCondition<T, TInfo::BASIC> : CopyAssignCondition<T> {
  using CopyAssignCondition<T>::CopyAssignCondition;

  constexpr MoveCondition() = default;
  constexpr MoveCondition(const MoveCondition& other) = default;
  constexpr MoveCondition& operator=(const MoveCondition&) = default;
  constexpr MoveCondition& operator=(MoveCondition&&) = default;

  constexpr MoveCondition(MoveCondition&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
    if (other.has_value()) {
      optional_impl<T>::construct(std::move(*other));
    }
  }
};

template <typename T>
struct MoveCondition<T, TInfo::DELETED> : CopyAssignCondition<T> {
  using CopyAssignCondition<T>::CopyAssignCondition;

  constexpr MoveCondition(MoveCondition&& other) = delete;

  constexpr MoveCondition() = default;
  constexpr MoveCondition(const MoveCondition& other) = default;
  constexpr MoveCondition& operator=(const MoveCondition&) = default;
  constexpr MoveCondition& operator=(MoveCondition&&) = default;
};

//--------------------------------------------- MOVE ASSIGN

template <typename T, TInfo COND = by_type<IsMoveable<T>::value, IsTriviallyMoveable<T>::value>()>
struct MoveAssignCondition : MoveCondition<T> {
  using MoveCondition<T>::MoveCondition;
};

template <typename T>
struct MoveAssignCondition<T, TInfo::BASIC> : MoveCondition<T> {
  using MoveCondition<T>::MoveCondition;

  constexpr MoveAssignCondition() = default;
  constexpr MoveAssignCondition(const MoveAssignCondition&) = default;
  constexpr MoveAssignCondition(MoveAssignCondition&&) = default;
  constexpr MoveAssignCondition& operator=(const MoveAssignCondition&) = default;

  constexpr MoveAssignCondition& operator=(MoveAssignCondition&& other
  ) noexcept(std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_constructible_v<T>) {
    if (other.has_value()) {
      if (optional_impl<T>::has_value()) {
        **this = std::move(*other);
      } else {
        optional_impl<T>::construct(std::move(*other));
      }
    } else {
      optional_impl<T>::reset();
    }
    return *this;
  }
};

template <typename T>
struct MoveAssignCondition<T, TInfo::DELETED> : MoveCondition<T> {
  using MoveCondition<T>::MoveCondition;

  constexpr MoveAssignCondition& operator=(MoveAssignCondition&&) = delete;

  constexpr MoveAssignCondition() = default;
  constexpr MoveAssignCondition(const MoveAssignCondition&) = default;
  constexpr MoveAssignCondition(MoveAssignCondition&&) = default;
  constexpr MoveAssignCondition& operator=(const MoveAssignCondition&) = default;
};
