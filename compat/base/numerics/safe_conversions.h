#ifndef ZUCCHINI_STANDALONE_BASE_NUMERICS_SAFE_CONVERSIONS_H_
#define ZUCCHINI_STANDALONE_BASE_NUMERICS_SAFE_CONVERSIONS_H_

#include <limits>
#include <type_traits>

#include "base/logging.h"

namespace base {

namespace internal {

template <typename T>
constexpr __int128 ToWide(T value) {
  static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
  if constexpr (std::is_enum_v<T>) {
    return ToWide(static_cast<std::underlying_type_t<T>>(value));
  } else if constexpr (std::is_signed_v<T>) {
    return static_cast<__int128>(value);
  } else {
    return static_cast<__int128>(static_cast<unsigned __int128>(value));
  }
}

template <typename T>
constexpr __int128 MinWide() {
  if constexpr (std::is_enum_v<T>) {
    return MinWide<std::underlying_type_t<T>>();
  } else if constexpr (std::is_signed_v<T>) {
    return static_cast<__int128>(std::numeric_limits<T>::lowest());
  } else {
    return 0;
  }
}

template <typename T>
constexpr __int128 MaxWide() {
  if constexpr (std::is_enum_v<T>) {
    return MaxWide<std::underlying_type_t<T>>();
  } else {
    return static_cast<__int128>(
        static_cast<unsigned __int128>(std::numeric_limits<T>::max()));
  }
}

}  // namespace internal

template <typename Destination, typename Source>
constexpr bool IsValueInRangeForNumericType(Source value) {
  static_assert((std::is_integral_v<Destination> || std::is_enum_v<Destination>) &&
                (std::is_integral_v<Source> || std::is_enum_v<Source>));
  const __int128 wide = internal::ToWide(value);
  return wide >= internal::MinWide<Destination>() &&
         wide <= internal::MaxWide<Destination>();
}

template <typename Destination, typename Source>
Destination checked_cast(Source value) {
  CHECK(IsValueInRangeForNumericType<Destination>(value));
  return static_cast<Destination>(value);
}

/* Chromium's strict_cast is a compile-time intent marker, not a runtime check. */
template <typename Destination, typename Source>
constexpr Destination strict_cast(Source value) {
  return static_cast<Destination>(value);
}

template <typename Destination, typename Source>
constexpr Destination saturated_cast(Source value) {
  if (internal::ToWide(value) < internal::MinWide<Destination>())
    return std::numeric_limits<Destination>::lowest();
  if (internal::ToWide(value) > internal::MaxWide<Destination>())
    return std::numeric_limits<Destination>::max();
  return static_cast<Destination>(value);
}

}  // namespace base

#endif
