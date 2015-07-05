#include "jpeg_dec.h"
#include <stdlib.h>

static int zero_base[] = {
	-1, -3, -7, -15, -31, -63, -127, -255, -511, -1023, -2047 };
static int one_base[] = {
	1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };

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

void parseScanData(uint8_t *ptr, jfif_info *j_info)
{
	int dc_val;
	initHuffParsing(ptr);

	// Parse Y block
	// Get Y DC val
	dc_val = getDcVal(&j_info->huff[0][0]);
	LOGD("The DC value is %d\n", dc_val);
	return;
}
