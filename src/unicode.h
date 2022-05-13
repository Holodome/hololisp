#ifndef __HLL_UNICODE_H__
#define __HLL_UNICODE_H__

#include "general.h"

/* 
 * Decodes from utf8 unicode codepoint
 * Retuns pointer to byte past decoded codepoint.
 * NOTE: Source buffer must always have 4 bytes aviable for reading.
 * This means that 3 additional zero bytes must be added to the end
 */
void const *utf8_decode_fast(void const *src, uint32_t *codepoint_p);

/*
 * Decodes unciode codepoint from utf8.
 * Returns pointer past read utf8 sequnce and writes codepoint to codepoint_p.
 * If given source is not correct utf8 sequnce, codepoint 0xFFFFFFFF is written
 */
void const *utf8_decode(void const *src, uint32_t *codepoint_p);

/*
 * Encodes unciode codepoint as utf8.
 * Writes encoded bytes to dst, returns pointer past last written byte
 */
void *utf8_encode(void *dst, uint32_t codepoint);

#endif
