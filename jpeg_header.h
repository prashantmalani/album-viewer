#ifndef _JPEG_HEADER_H_
#define _JPEG_HEADER_H_

#include <stdint.h>

/*
 * Structure representing the JFIF header segment (APP0).
 * This contains basic information about the image,
 * including image size, thumbnail size etc.
 * We parse this header to ensure that the file being decoded contains
 * valid JPEG data.
 */
typedef struct {
	uint16_t	soi;
	uint16_t	app0;
	uint16_t	app0_len;
	uint8_t		id_str[5];  /* Null-terminated string "JFIF". */
	uint16_t	version;
	uint8_t		units;
	uint16_t	x_res;
	uint16_t	y_res;
	uint8_t		x_thumb;
	uint8_t		y_thumb;
} __attribute__((__packed__)) j_header ;

/*
 * FUNCTION parseHeader
 *
 * Parse the header to get initial metadata about image.
 *
 * Args:
 *  b_arr : Byte array containing raw file data.
 *  header_ptr: pointer to a header structure which needs to be filled up.
 *
 * Returns
 *  0 on success (with |header_ptr| populated
 *  -1 otherwise.
 */
int parseHeader(uint8_t *b_arr, j_header *header_ptr);

#endif
