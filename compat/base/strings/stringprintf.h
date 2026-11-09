#ifndef ZUCCHINI_STANDALONE_BASE_STRINGS_STRINGPRINTF_H_
#define ZUCCHINI_STANDALONE_BASE_STRINGS_STRINGPRINTF_H_

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

namespace base {

inline std::string StringPrintV(const char* format, va_list arguments) {
  va_list copy;
  va_copy(copy, arguments);
  const int length = std::vsnprintf(nullptr, 0, format, copy);
  va_end(copy);
  if (length < 0)
    return {};

  std::vector<char> buffer(static_cast<size_t>(length) + 1);
  std::vsnprintf(buffer.data(), buffer.size(), format, arguments);
  return std::string(buffer.data(), static_cast<size_t>(length));
}

inline std::string StringPrintf(const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  std::string result = StringPrintV(format, arguments);
  va_end(arguments);
  return result;
}

}  // namespace base

#endif
