#ifndef JPEG_DEC_H
#define JPEG_DEC_H

#include <stdint.h>

typedef struct {
	uint16_t	soi;
	uint16_t	app0;
	uint16_t	app0_len;
	// Null terminated string "JFIF"
	uint8_t		id_str[5];
	uint16_t	version;
	uint8_t		units;
	uint16_t	x_res;
	uint16_t	y_res;
	uint8_t		x_pixel;
	uint8_t		y_pixel;
} __attribute__((__packed__)) jfif_header ;

typedef struct {
	jfif_header	hdr;
} jfif_info;

#endif

