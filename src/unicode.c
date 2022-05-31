#include "unicode.h"

void const *
utf8_decode(void const *srcv, uint32_t *utf32) {
    uint8_t const *src = srcv;

    if ((src[0] & 0x80) == 0x00) {
        *utf32 = src[0];
        ++src;
    } else if ((src[0] & 0xE0) == 0xC0 && (src[1] & 0xC0) == 0x80) {
        *utf32 = ((src[0] & 0x1F) << 6) | (src[1] & 0x3F);
        src += 2;
    } else if ((src[0] & 0xF0) == 0xE0 && (src[1] & 0xC0) == 0x80 &&
               (src[2] & 0xC0) == 0x80) {
        *utf32 =
            ((src[0] & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
        src += 3;
    } else if ((src[0] & 0xF8) == 0xF0 && (src[1] & 0xC0) == 0x80 &&
               (src[2] & 0xC0) == 0x80 && (src[3] & 0xC0) == 0x80) {
        *utf32 = ((src[0] & 0x03) << 18) | ((src[1] & 0x3F) << 12) |
                 ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
        src += 4;
    }

    return src;
}

void *
utf8_encode(void *dst, uint32_t cp) {
    uint8_t *d = (uint8_t *)dst;
    if (cp <= 0x0000007F) {
        *d++ = cp;
    } else if (cp <= 0x000007FF) {
        *d++ = 0xC0 | (cp >> 6);
        *d++ = 0x80 | (cp & 0x3F);
    } else if (cp <= 0x0000FFFF) {
        *d++ = 0xE0 | (cp >> 12);
        *d++ = 0x80 | ((cp >> 6) & 0x3F);
        *d++ = 0x80 | (cp & 0x3F);
    } else if (cp <= 0x0010FFFF) {
        *d++ = 0xF0 | (cp >> 18);
        *d++ = 0x80 | ((cp >> 12) & 0x3F);
        *d++ = 0x80 | ((cp >> 6) & 0x3F);
        *d++ = 0x80 | (cp & 0x3F);
    }
    return (void *)d;
}
