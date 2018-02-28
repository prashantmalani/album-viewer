#ifndef JPEG_DEC_H
#define JPEG_DEC_H

#include <stdint.h>
#include <stdbool.h>
#include "jpeg_header.h"
#include "debug.h"

#define APP_MASK	0xFF00
#define DQT_MARKER	0xFFDB
#define SOF_MARKER	0xFFC0
#define HUF_MARKER	0xFFC4
#define SOS_MARKER	0xFFDA

// Define Quantization table
// TODO(pmalani): Need to account for multiple QT
typedef struct {
	uint16_t	lq;
	uint8_t		pq; // QT precision
	uint8_t		tq; // QT identifier
	uint8_t		el[64]; // QT elements
} jfif_dqt;

typedef struct {
	uint8_t		c; // Component indentifier
	uint8_t		h; // Horizontal sampling factor
	uint8_t		v; // Veritical sampling factor
	uint8_t		qt_sel; // Quantization table selector
} jfif_sof_comp;

typedef struct {
	uint16_t	len;
	uint8_t		prec;
	uint16_t	y;
	uint16_t	x;
	uint8_t		num_f;
	jfif_sof_comp	comp[3]; // For Y U and V
} jfif_sof;

typedef struct node {
	uint8_t 	val;
	struct node	*l;
	struct node	*r;
} node_t;

typedef struct {
	uint16_t	len;
	uint8_t		tclass;
	uint8_t		tid;
	uint8_t		l[16]; // Num of codes for each length
	uint8_t		*codes; // Array holding actual codes
	node_t		*root;

} jfif_huff;

typedef struct {
	uint8_t		num_c; // Number of image components
	uint8_t		dc_sel[3]; // Per comp. DC coding table selector. 
	uint8_t		ac_sel[3]; // Per comp. AC coding table selector.
} jfif_sos;

typedef struct {
	j_header	hdr;
	jfif_dqt	dqt[2];
	bool		one_dqt;
	jfif_sof	sof;
	jfif_huff	huff[2][2]; // Keep 4 since we assume 2 each
				 // for Y and Cb/Cr respectively.
	jfif_sos	sos;

	// RGB 2-D Array pointers. Need to be initialized.
	uint8_t		**r;
	uint8_t		**g;
	uint8_t		**b;
} jfif_info;

/* Huff stuff */
void genHuff(jfif_huff *huf);
uint16_t getNumBits(uint8_t num);
void initHuffParsing(uint8_t *ptr);
uint8_t getVal(jfif_huff *huff);

/* Scan data stuff */
void parseScanData(uint8_t *ptr, jfif_info *j_info);

/* TODO: Remove later, only for debug purposes */
void writeToBmp(uint8_t **red, uint8_t **green, uint8_t **blue, int width,
		int height);


#endif

