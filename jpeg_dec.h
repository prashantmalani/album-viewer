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

// Define Quantization table
// TODO(pmalani): Need to account for multiple QT
typedef struct {
	uint16_t	len;
	uint8_t		pq; // QT precision
	uint8_t		tq; // QT identifier
	uint8_t		el[64]; // QT elements
} __attribute__((__packed__)) jfif_dqt;

typedef struct {
	jfif_header	hdr;
	jfif_dqt	dqt;
} jfif_info;

#endif

