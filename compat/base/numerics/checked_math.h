#ifndef ZUCCHINI_STANDALONE_BASE_NUMERICS_CHECKED_MATH_H_
#define ZUCCHINI_STANDALONE_BASE_NUMERICS_CHECKED_MATH_H_

#include <limits>
#include <type_traits>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"

namespace base {

template <typename T>
class CheckedNumeric {
  static_assert(std::is_integral_v<T>);

 public:
  constexpr CheckedNumeric() = default;

  template <typename U,
            typename = std::enable_if_t<std::is_integral_v<U> ||
                                        std::is_enum_v<U>>>
  constexpr CheckedNumeric(U value) {
    Assign(value);
  }

  template <typename U>
  constexpr CheckedNumeric(const CheckedNumeric<U>& other) {
    valid_ = other.IsValid() &&
             IsValueInRangeForNumericType<T>(other.ValueOrDefault(0));
    if (valid_)
      value_ = static_cast<T>(other.ValueOrDefault(0));
  }

  constexpr bool IsValid() const { return valid_; }

  constexpr T ValueOrDefault(T fallback) const {
    return valid_ ? value_ : fallback;
  }

  T ValueOrDie() const {
    CHECK(valid_);
    return value_;
  }

  template <typename D>
  D ValueOrDie() const {
    CHECK(valid_);
    return checked_cast<D>(value_);
  }

  template <typename U>
  constexpr bool AssignIfValid(U* output) const {
    if (!valid_ || output == nullptr ||
        !IsValueInRangeForNumericType<U>(value_)) {
      return false;
    }
    *output = static_cast<U>(value_);
    return true;
  }

  template <typename U>
  constexpr CheckedNumeric& operator=(U value) {
    Assign(value);
    return *this;
  }

  template <typename U>
  constexpr CheckedNumeric operator+(U rhs) const {
    return Binary(rhs, '+');
  }

  template <typename U>
  constexpr CheckedNumeric operator-(U rhs) const {
    return Binary(rhs, '-');
  }

  template <typename U>
  constexpr CheckedNumeric operator*(U rhs) const {
    return Binary(rhs, '*');
  }

  template <typename U>
  constexpr CheckedNumeric operator/(U rhs) const {
    CheckedNumeric result;
    if (!valid_) {
      result.valid_ = false;
      return result;
    }
    const __int128 divisor = Wide(rhs, &result.valid_);
    if (!result.valid_ || divisor == 0) {
      result.valid_ = false;
      return result;
    }
    const __int128 quotient = internal::ToWide(value_) / divisor;
    result.AssignWide(quotient);
    return result;
  }

  template <typename U>
  constexpr CheckedNumeric& operator+=(U rhs) {
    *this = *this + rhs;
    return *this;
  }

  template <typename U>
  constexpr CheckedNumeric& operator-=(U rhs) {
    *this = *this - rhs;
    return *this;
  }

  template <typename U>
  constexpr CheckedNumeric& operator*=(U rhs) {
    *this = *this * rhs;
    return *this;
  }

  template <typename U>
  constexpr CheckedNumeric& operator/=(U rhs) {
    *this = *this / rhs;
    return *this;
  }

 private:
  template <typename U>
  constexpr void Assign(U value) {
    valid_ = IsValueInRangeForNumericType<T>(value);
    if (valid_)
      value_ = static_cast<T>(value);
  }

  constexpr void AssignWide(__int128 value) {
    valid_ = value >= internal::MinWide<T>() &&
             value <= internal::MaxWide<T>();
    if (valid_)
      value_ = static_cast<T>(value);
  }

  template <typename U>
  static constexpr __int128 Wide(const U& value, bool* valid) {
    if constexpr (std::is_same_v<std::decay_t<U>, CheckedNumeric<T>>) {
      *valid = value.IsValid();
      return value.IsValid() ? internal::ToWide(value.ValueOrDefault(0)) : 0;
    } else if constexpr (std::is_integral_v<std::decay_t<U>> ||
                         std::is_enum_v<std::decay_t<U>>) {
      *valid = true;
      return internal::ToWide(value);
    } else {
      *valid = value.IsValid();
      return value.IsValid() ? internal::ToWide(value.ValueOrDefault(0)) : 0;
    }
  }

  template <typename U>
  constexpr CheckedNumeric Binary(const U& rhs, char operation) const {
    CheckedNumeric result;
    bool rhs_valid = true;
    const __int128 right = Wide(rhs, &rhs_valid);
    if (!valid_ || !rhs_valid) {
      result.valid_ = false;
      return result;
    }

    const __int128 left = internal::ToWide(value_);
    __int128 output = 0;
    switch (operation) {
      case '+': output = left + right; break;
      case '-': output = left - right; break;
      case '*': output = left * right; break;
      default: result.valid_ = false; return result;
    }
    result.AssignWide(output);
    return result;
  }

  T value_ = 0;
  bool valid_ = true;
};

template <typename U, typename T,
          typename = std::enable_if_t<std::is_integral_v<U> ||
                                      std::is_enum_v<U>>>
constexpr CheckedNumeric<T> operator+(U lhs, const CheckedNumeric<T>& rhs) {
  return CheckedNumeric<T>(lhs) + rhs;
}

template <typename U, typename T,
          typename = std::enable_if_t<std::is_integral_v<U> ||
                                      std::is_enum_v<U>>>
constexpr CheckedNumeric<T> operator-(U lhs, const CheckedNumeric<T>& rhs) {
  return CheckedNumeric<T>(lhs) - rhs;
}

template <typename U, typename T,
          typename = std::enable_if_t<std::is_integral_v<U> ||
                                      std::is_enum_v<U>>>
constexpr CheckedNumeric<T> operator*(U lhs, const CheckedNumeric<T>& rhs) {
  return CheckedNumeric<T>(lhs) * rhs;
}

template <typename T>
constexpr CheckedNumeric<T> MakeCheckedNum(T value) {
  return CheckedNumeric<T>(value);
}

template <typename A, typename B>
constexpr auto CheckAdd(A a, B b) {
  using R = std::common_type_t<A, B>;
  return CheckedNumeric<R>(a) + b;
}

template <typename A, typename B>
constexpr auto CheckSub(A a, B b) {
  using R = std::common_type_t<A, B>;
  return CheckedNumeric<R>(a) - b;
}

template <typename A, typename B>
constexpr auto CheckMul(A a, B b) {
  using R = std::common_type_t<A, B>;
  return CheckedNumeric<R>(a) * b;
}

}  // namespace base

#endif
