#ifndef ZUCCHINI_STANDALONE_BASE_BIND_H_
#define ZUCCHINI_STANDALONE_BASE_BIND_H_

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace base {

template <typename T>
T* Unretained(T* pointer) {
  return pointer;
}

template <typename F, typename... BoundArgs>
auto BindRepeating(F&& function, BoundArgs&&... bound_args) {
  return [function = std::forward<F>(function),
          bound = std::make_tuple(std::forward<BoundArgs>(bound_args)...)](
             auto&&... remaining_args) mutable -> decltype(auto) {
    return std::apply(
        [&](auto&&... stored_args) -> decltype(auto) {
          return std::invoke(function,
                             std::forward<decltype(stored_args)>(stored_args)...,
                             std::forward<decltype(remaining_args)>(
                                 remaining_args)...);
        },
        bound);
  };
}

template <typename F, typename... BoundArgs>
auto BindOnce(F&& function, BoundArgs&&... bound_args) {
  return BindRepeating(std::forward<F>(function),
                       std::forward<BoundArgs>(bound_args)...);
}

}  // namespace base

#endif  // ZUCCHINI_STANDALONE_BASE_BIND_H_
