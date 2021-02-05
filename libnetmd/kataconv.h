#ifndef LIBNETMD_KATACONV_H
#define LIBNETMD_KATACONV_H

#include <stdint.h>

/**
   Rewrite full-width katakana to half-width in place.

   @param input pointer to UTF-8 encoded string to convert
*/
void kata_full_to_half(uint8_t* input);

/**
   Rewrite half-width katakana to full-width in place.

   @param input pointer to UTF-8 encoded string to convert
*/
void kata_half_to_full(uint8_t* input);

#endif
