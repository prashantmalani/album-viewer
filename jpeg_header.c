/*
 * jpeg_header.c
 *
 * File that contains code to parse the JFIF header correctly.
 *
 * Autor: Prashant Malani <p.malani@gmail.com>
 * Date : 01/03/2018
 *
 */

#include <string.h>

#include "jpeg_header.h"
#include "debug.h"

/*
 * FUNCTION sanitizeScanData
 *
 * Parse the scan data, and remove all app markers which were added for
 * data integrity.
 *
 * The original data is pointed by |ptr| and the sanitized data should
 * be placed in the array pointed to by |clean_result|.
 *
 * It is safe to assume that the caller has allocated the requisite
 * memory for both |ptr| and |clean_result|.
 */
void sanitizeScanData(uint8_t *ptr, uint8_t *clean_result)
{
	uint8_t *new_ptr = clean_result;
	uint16_t cur_short = swapBytes(*(uint16_t *)ptr);
	LOGD("Starting scan data sanitization\n");
	while (cur_short != EOI_MARKER) {
		if (*ptr != 0xFF) {
			*new_ptr++ = *ptr++;
		} else {
			if (*(ptr + 1) == 0x00)
				*new_ptr++ = 0xFF;
			ptr += 2;
		}
		cur_short = swapBytes(*(uint16_t *)ptr);
	}
	memcpy(new_ptr, ptr, sizeof(uint16_t));
	LOGD("Completed sanitization successfully!");
	return;
}

int parseHeader(uint8_t *array, uint8_t *clean_array, j_header *header_ptr)
{
	int i;
	uint8_t *cur_ptr;
	uint8_t jump_len;
	uint16_t new_app0;

	// Cast to a j_header, and then parse.
	j_header *tmp_hdr = (j_header *)array;

	header_ptr->soi = swapBytes(tmp_hdr->soi);
	if (header_ptr->soi != SOI_MARKER)
		return -1;

	header_ptr->app0 = swapBytes(tmp_hdr->app0);
	if (header_ptr->app0 != APP0_MARKER)
		return -1;

	header_ptr->app0_len = swapBytes(tmp_hdr->app0_len);
	LOGD("The APP0 segment length is %u\n", header_ptr->app0_len);

	memcpy(header_ptr->id_str, tmp_hdr->id_str, sizeof(tmp_hdr->id_str) /
			sizeof(tmp_hdr->id_str[0]));
	if (strcmp((char *)header_ptr->id_str, "JFIF"))
		return -1;

	header_ptr->version = tmp_hdr->version;
	LOGD("The version is %04x\n", header_ptr->version);

	header_ptr->x_res = swapBytes(tmp_hdr->x_res);
	LOGD("The X resolution is %u\n", header_ptr->x_res);

	header_ptr->y_res = swapBytes(tmp_hdr->y_res);
	LOGD("The y resolution is %u\n", header_ptr->y_res);

	header_ptr->units= tmp_hdr->units;
	LOGD("The units is %u\n", header_ptr->units);

	header_ptr->x_thumb = tmp_hdr->x_thumb;
	LOGD("The thumbnail X size is %u\n", header_ptr->x_thumb);

	header_ptr->y_thumb = tmp_hdr->y_thumb;
	LOGD("The thumbnail Y size is %u\n", header_ptr->y_thumb);

	if (parseDqt(&cur_ptr)) {
		LOGE("Error parsing QT\n");
		return -1;
	}

	if (parseSof(&cur_ptr)) {
		LOGE("Error parsing SOF\n");
		return -1;
	}

	if (parseHuff(&cur_ptr)) {
		LOGE("Error parsing Huffman Table.\n");
		return -1;
	}

	if (parseSos(&cur_ptr)) {
		LOGE("Error parsing SOS Table\n");
		return -1;
	}

	sanitizeScanData(cur_ptr, clean_array);

	// Print out sanitized contents for debug purposes.
	LOGD("\n");
	for (i = 0; swapBytes(*(uint16_t *)(clean_array + i)) != EOI_MARKER;
		i++) {
		LOGD("%02x ", clean_array[i]);
		if (!((i+1) % 16))
			LOGD("\n");
	}
	LOGD("\n");

	return 0;
}
