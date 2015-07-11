#include "jpeg_dec.h"
#include <stdlib.h>
#include <string.h>

static int zero_base[] = {
	-1, -3, -7, -15, -31, -63, -127, -255, -511, -1023, -2047 };
static int one_base[] = {
	1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };
static int dezigzag_index[] = {
	0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12,
	19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35,
	42, 49, 56, 57, 50, 43, 67, 29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62,
	63 };

int getDcVal(jfif_huff *huff)
{
	uint8_t num_bits = getVal(huff);
	int val;
	uint16_t dc_code;
	if (num_bits == 0)
		return 0;

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

void parseScanData(uint8_t *ptr, jfif_info *j_info)
{
	int dc_val = 0, i = 0; /* The previous block dc_val is o */
	int block_arr[64] = {0};
	int8_t val;

	initHuffParsing(ptr);

	/* Parse Y block */
	/* Get Y DC val */
	dc_val += getDcVal(&j_info->huff[0][0]);
	LOGD("The DC value is %d\n", dc_val);

	block_arr[i++] = dc_val;

	while ((val = getVal(&j_info->huff[1][0])) || i == 64) {
		LOGD("Got an AC value=%d , index %d\n", (int)val, i++);
		// TODO(pmalani): add quantization table multiplication.
		block_arr[i++] = (int)val;

	}
	dezigzagBlock(dezigzag_index, block_arr);

	return;
}
