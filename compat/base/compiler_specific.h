#ifndef ZUCCHINI_STANDALONE_BASE_COMPILER_SPECIFIC_H_
#define ZUCCHINI_STANDALONE_BASE_COMPILER_SPECIFIC_H_
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define FALLTHROUGH [[fallthrough]]
#define PRINTF_FORMAT(format_param, dots_param) \
  __attribute__((format(printf, format_param, dots_param)))
#endif
