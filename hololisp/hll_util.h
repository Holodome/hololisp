#ifndef HLL_UTIL_H
#define HLL_UTIL_H

#if defined(__GNUC__) || defined(__clang__)
#define HLL_LIKELY(...) __builtin_expect((__VA_ARGS__), true)
#else
#warn Unsupported compiler for HLL_LIKELY
#define HLL_LIKELY(...) (__VA_ARGS__)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define HLL_UNLIKELY(...) __builtin_expect((__VA_ARGS__), false)
#else
#warn Unsupported compiler for HLL_UNLIKELY
#define HLL_UNLIKELY(...) (__VA_ARGS__)
#endif

#ifdef HLL_DEBUG
#if defined(__GNUC__) || defined(__clang__)
#define HLL_UNREACHABLE __builtin_trap()
#else
#error Not implemented
#endif
#else // HLL_DEBUG
#if defined(__GNUC__) || defined(__clang__)
#define HLL_UNREACHABLE __builtin_unreachable()
#else
#error Not implemented
#endif
#endif // HLL_DEBUG

#if defined(__GNUC__) || defined(__clang__)
#define HLL_ATTR(...) __attribute__((__VA_ARGS__))
#else
#define HlL_ATTR(...)
#endif

#endif
