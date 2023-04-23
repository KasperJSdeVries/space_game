#pragma once

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;

typedef float f32;
typedef double f64;

typedef int b32;
typedef _Bool b8;

typedef struct range {
  u64 offset;
  u64 size;
} range;

#if defined(__clang__) || defined(__GNUC__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

#define true 1
#define false 0

STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 bytes.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 bytes.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

STATIC_ASSERT(sizeof(b32) == 4, "Expected b32 to be 4 bytes.");
STATIC_ASSERT(sizeof(b8) == 1, "Expected b8 to be 1 bytes.");

#define INVALID_ID_U64 18446744073709551615UL
#define INVALID_ID 4294967295U
#define INVALID_ID_U16 65535U
#define INVALID_ID_U8 255U

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define SPACE_PLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define SPACE_PLATFORM_LINUX 1
#if defined(__ANDROID__)
#define SPACE_PLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define SPACE_PLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define SPACE_PLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define SPACE_PLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define SPACE_PLATFORM_IOS 1
#define SPACE_PLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define SPACE_PLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#if SPACE_EXPORT
#ifdef _MSC_VER
#define SPACE_API __declspec(dllexport)
#else
#define SPACE_API __attribute__((visibility("default")))
#endif
#else
#ifdef _MSC_VER
#define SPACE_API __declspec(dllimport)
#else
#define SPACE_API
#endif
#endif

#define SPACE_CLAMP(value, min, max)                                           \
  ((value <= min) ? min : (value >= max) ? max : value)

// inlining
#if defined(__clang__) || defined(__gcc__)
#define SPACE_INLINE __attribute__((always_inline)) inline

#define SPACE_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define SPACE_INLINE __forceinline

#define SPACE_NOINLINE __declspec(noinline)
#else
#define SPACE_INLINE static inline

#define SPACE_NOINLINE
#endif

#define GIBIBYTES(amount) ((amount)*1024ULL * 1024ULL * 1024ULL)
#define MEBIBYTES(amount) ((amount)*1024ULL * 1024ULL)
#define KIBIBYTES(amount) ((amount)*1024ULL)

#define GIGABYTES(amount) ((amount)*1000ULL * 1000ULL * 1000ULL)
#define MEGABYTES(amount) ((amount)*1000ULL * 1000ULL)
#define KILOBYTES(amount) ((amount)*1000ULL)

SPACE_INLINE u64 get_aligned(u64 operand, u64 granularity) {
  return ((operand + (granularity - 1)) & ~(granularity - 1));
}

SPACE_INLINE range get_aligned_range(u64 offset, u64 size, u64 granularity) {
  return (range){get_aligned(offset, granularity),
                 get_aligned(size, granularity)};
}

#define SPACE_MIN(x, y) (x < y ? x : y)
#define SPACE_MAX(x, y) (x > y ? x : y)
