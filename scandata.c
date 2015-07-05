#include "jpeg_dec.h"
#include <stdlib.h>

uint16_t getDcVal(jfif_huff *huff)
{
	// First get the code from huff
	uint8_t val = getVal(huff);
	LOGD("The number of bits to check is %u\b", val);
	return (uint16_t)val;
}

void parseScanData(uint8_t *ptr, jfif_info *j_info)
{
	uint16_t dc_val;
	initHuffParsing(ptr);

	// Parse Y block
	// Get Y DC val
	dc_val = getDcVal(&j_info->huff[0][0]);
	LOGD("Y DC val is %u\n", dc_val);
	return;
}
