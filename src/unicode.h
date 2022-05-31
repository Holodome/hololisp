#ifndef __HLL_UNICODE_H__
#define __HLL_UNICODE_H__

#include <stdint.h>

/// \brief Decodes unicode codepoint from utf8-encoded data stream.
/// \param src Data to decode from
/// \param utf32 Pointer where decoded codepoint should be written.
/// \returns Data pointer past read codepoint. If data point is not correct
/// utf8-encoded data pointer is not changed.
void const *utf8_decode(void const *src, uint32_t *utf32);

/// \brief Encodes unicode codepoint to utf8.
/// \param dst Utf8-encoded destination buffer. Must contain at least 4 bytes.
/// \param codepoint utf32 codepoint to encode to utf8.
/// \returns Pointer one byte past encoded data.
void *utf8_encode(void *dst, uint32_t codepoint);

#endif
