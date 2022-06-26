#ifndef __HLL_EXT_H__
#define __HLL_EXT_H__

#if defined(__GNUC__) || defined(__clang__)
#define HLL_ATTR(...) __attribute__((__VA_ARGS__))
#else 
#define HlL_ATTR(...)
#endif 

#if defined(__cplusplus)
#define HLL_NODISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
#define HLL_NODISCARD __attribute__((warn_unused_result))
#else 
#define HLL_NODISCARD 
#endif 

#if defined(__cplusplus)
#define HLL_DECL extern "C"
#else
#define HLL_DECL extern
#endif 

#endif 
