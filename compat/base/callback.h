#ifndef ZUCCHINI_STANDALONE_BASE_CALLBACK_H_
#define ZUCCHINI_STANDALONE_BASE_CALLBACK_H_

#include <functional>
#include <type_traits>
#include <utility>

namespace base {

template <typename Signature>
class RepeatingCallback;

template <typename R, typename... Args>
class RepeatingCallback<R(Args...)> {
 public:
  RepeatingCallback() = default;
  RepeatingCallback(std::nullptr_t) {}

  template <typename F,
            typename = std::enable_if_t<!std::is_same_v<
                std::decay_t<F>, RepeatingCallback<R(Args...)>>>>
  RepeatingCallback(F&& function) : function_(std::forward<F>(function)) {}

  R Run(Args... args) const {
    return function_(std::forward<Args>(args)...);
  }

  explicit operator bool() const { return static_cast<bool>(function_); }
  bool is_null() const { return !function_; }
  void Reset() { function_ = nullptr; }

 private:
  std::function<R(Args...)> function_;
};

template <typename Signature>
using OnceCallback = RepeatingCallback<Signature>;

using RepeatingClosure = RepeatingCallback<void()>;
using OnceClosure = OnceCallback<void()>;

}  // namespace base

#endif  // ZUCCHINI_STANDALONE_BASE_CALLBACK_H_
