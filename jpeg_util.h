#ifndef _JPEG_UTIL_H_
#define _JPEG_UTIL_H_

#include <stdint.h>

uint16_t swapBytes(uint16_t val)
{
	uint16_t ret_val;
	uint8_t *ptr = (uint8_t *)&val;
	ret_val = *ptr << CHAR_BIT | *(ptr + 1);
	return ret_val;
}

#endif // _JPEG_UTIL_H_
