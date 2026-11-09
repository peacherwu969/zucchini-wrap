#ifndef ZUCCHINI_STANDALONE_BASE_DEBUG_STACK_TRACE_H_
#define ZUCCHINI_STANDALONE_BASE_DEBUG_STACK_TRACE_H_

#include <string>

namespace base::debug {

class StackTrace {
 public:
  StackTrace() = default;
  std::string ToString() const {
    return "stack trace unavailable in standalone build";
  }
};

}  // namespace base::debug

#endif
