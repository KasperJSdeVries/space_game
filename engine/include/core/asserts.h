#pragma once

#include "defines.h"

#ifndef SPACE_ASSERTIONS_ENABLED
#define SPACE_ASSERTIONS_ENABLED 1
#endif

#if SPACE_ASSERTIONS_ENABLED == 1
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

SPACE_API void report_assertion_failure(const char *expression,
                                        const char *message, const char *file,
                                        int32_t line);

#define SPACE_ASSERT(expr)                                                     \
  {                                                                            \
    if (!(expr)) {                                                             \
      report_assertion_failure(#expr, "", __FILE__, __LINE__);                 \
      debugBreak();                                                            \
    }                                                                          \
  }

#define SPACE_ASSERT_MESSAGE(expr, message)                                    \
  {                                                                            \
    if (!(expr)) {                                                             \
      report_assertion_failure(#expr, message, __FILE__, __LINE__);            \
      debugBreak();                                                            \
    }                                                                          \
  }

#ifdef _DEBUG
#define SPACE_ASSERT_DEBUG(expr)                                               \
  {                                                                            \
    if (!(expr)) {                                                             \
      report_assertion_failure(#expr, "", __FILE__, __LINE__);                 \
      debugBreak();                                                            \
    }                                                                          \
  }
#else
#define SPACE_ASSERT_DEBUG(expr)
#endif

#else
#define SPACE_ASSERT(expr)
#define SPACE_ASSERT_MESSAGE(expr)
#define SPACE_ASSERT_DEBUG(expr)
#endif
