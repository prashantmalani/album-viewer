#include "jpeg_dec.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

static int zero_base[] = {
	-1, -3, -7, -15, -31, -63, -127, -255, -511, -1023, -2047 };
static int one_base[] = {
	1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
static int dezigzag_index[] = {
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12,
	19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35,
	42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62,
	63 };

static int dc_val;

static int getDcVal(jfif_huff *huff)
{
	uint8_t num_bits = getVal(huff);
	int val;
	uint16_t dc_code;
	if (num_bits == 0)
		return INT_MIN;

	LOGD("The number of bits to check is %u\n", num_bits);
	dc_code = getNumBits(num_bits);

	LOGD("The DC code is %x\n", dc_code);

	// Check the MSB; and set base value accordingly.
	if ((dc_code >> (num_bits - 1)) & 0x1)
		val = one_base[num_bits - 1];
	else
		val = zero_base[num_bits - 1];

	val += (dc_code & ~(1 << (num_bits - 1)));
	return val;
}

static void dezigzagBlock(const int index[], int block[])
{
	int *temp_arr = malloc(sizeof(int) * 64);
	int i;
	for (i = 0; i < 64; i++) {
		temp_arr[i] = block[index[i]];
	}

	memcpy(block, temp_arr, sizeof(int) * 64);
	free(temp_arr);
}

static void dequantizeBlock(jfif_dqt *dqt, int block[])
{
	int i;
	for (i = 0; i< 64; i++) {
		block[i] *= dqt->el[i];
	}
}

static inline float coeffX(int x)
{
	return (!x ? 0.707 : 1);
}

void doIdct(int block[])
{
	float idct[64];
	int x, y, i;
	int u,v;
	float sum, f_uv;
	for (i = 0; i < 64; i++) {
		x = i % 8;
		y = i / 8;
		sum = 0;
		for (u = 0; u < 8; u++) {
			for (v = 0; v < 8; v++) {
				f_uv = (float)block[u + (v * 8)];

				sum +=  coeffX(u) * coeffX(v) * f_uv *
					cos((2 * x + 1) * (float)u * M_PI / 16) *
					cos((2 * y + 1) * (float)v * M_PI / 16);
			}
		}

		// We want to round to the nearest integer.
		idct[i] = rintf(0.25 * sum);
		LOGD("IDCT value for %d,%d = %f\n", x, y, idct[i]);
	}
	// Conver to integral values
	for (i = 0; i < 64; i++) {
		block[i] = idct[i];
	}
}

void parseComponent(jfif_info *j_info, int block[], int comp)
{
	int i = 0;
	int8_t val;
	int dc_diff;

	/*  DC val */
	dc_diff = getDcVal(&j_info->huff[0][comp]);
	if (dc_diff == INT_MIN)
		goto skip_parsing_dc;

	dc_val += dc_diff;
	LOGD("The DC value is %d\n", dc_val);

	block[0] = dc_val;

skip_parsing_dc:
	i = 1;
	while ((val = getVal(&j_info->huff[1][comp])) && i != 64) {
		LOGD("Got an AC value=%d , index %d\n", (int)val, i++);
		block[i++] = (int)val;
	}

	// First multiply by quantization table ?
	dequantizeBlock(&j_info->dqt[comp], block);
	dezigzagBlock(dezigzag_index, block);

	// Perform IDCT
	doIdct(block);

	// Shift up each value by 128
	for (i = 0; i < 64; i++)
		block[i] += 128;
}

void convertToRgb(int *bk_y, int *bk_cb, int *bk_cr, int x, int y,
                  jfif_info *j_info)
{
	int i, j, k;
	uint8_t **r = j_info->r;
	uint8_t **g = j_info->g;
	uint8_t **b = j_info->b;

	// NOTE: the y cordinate would map to the first index
	for (i = 0; i < 64; i++) {
		j = i % 8;
		k = i / 8;
		r[y + j][x + k] = (uint8_t)(bk_y[i] + ((1402 * (bk_cr[i] -
			128)) / 1000));
		g[y + j][x + k] = (uint8_t)(bk_y[i] - ((344 * (bk_cb[i] -
			128)) / 1000) - ((714 * (bk_cr[i] - 128)) / 1000));
		b[y + j][x + k] = (uint8_t)(bk_y[i] + ((1772 * (bk_cb[i] -
			128)) / 1000));
		LOGD("R=%u, G=%u, b=%u\n", r[y + j][x + k], g[y + j][x + k],
			b[y +j][x + k]);
	}
}

void parseBlock(jfif_info *j_info, int w_off, int h_off)
{
	static int block_y[64] = {0}, block_cb[64] = {0}, block_cr[64] = {0};

	memset(block_y, 0, sizeof(block_y));
	memset(block_cb, 0, sizeof(block_cb));
	memset(block_cr, 0, sizeof(block_cr));

	// Parse Y block.
	LOGD("Parsing Y component\n");
	parseComponent(j_info, block_y, 0);

	// Parse Cb block.
	LOGD("Parsing Cb component\n");
	parseComponent(j_info, block_cb, 1);

	// Parse Cr block.
	LOGD("Parsing Cr component\n");
	parseComponent(j_info, block_cr, 1);

	convertToRgb(block_y, block_cb, block_cr, w_off, h_off, j_info);
	return;
}

void parseScanData(uint8_t *ptr, jfif_info *j_info)
{
	int i = 0; /* The previous block dc_val is o */
	int w_off, h_off;

	initHuffParsing(ptr);

	dc_val = 0;

	LOGD("Parsing second MCU\n");
	for (h_off = 0; h_off < j_info->sof.y; h_off += 8) {
		for (w_off = 0; w_off < j_info->sof.x; w_off += 8) {
			parseBlock(j_info, w_off, h_off);
		}
	}
	return;
}
