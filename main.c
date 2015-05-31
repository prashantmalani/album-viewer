/*
 * main.c
 *
 * Main file for execution of the JPEG decoder
 *
 * Author: Prashant Malani <p.malani@gmail.com>
 * Date  : 04/20/2015
 *
 */
#include "debug.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "jpeg_dec.h"

/* Array which stores the raw JPEG bytes */
uint8_t *bArray;

jfif_info *jInfo;

int loadFile(char *path)
{
	FILE *fp = NULL;
	long int fsize;
	int i;

	fp = fopen(path, "rb");
	if (!fp) {
		LOGE("Error opening file %s\n", path);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	bArray = malloc(fsize * sizeof(uint8_t));
	fread(bArray, fsize, 1, fp);
	fclose(fp);

	// Print out file contents for debug purposes.
	LOGD("\n");
	for (i = 0; i < fsize; i++) {
		LOGD("%02x ", bArray[i]);
		if (!((i+1) % 16))
			LOGD("\n");
	}

	LOGD("\n");
	return 0;
}

uint16_t swapBytes(uint16_t val)
{
	uint16_t ret_val;
	uint8_t *ptr = (uint8_t *)&val;
	ret_val = *ptr << CHAR_BIT | *(ptr + 1);
	return ret_val;
}

/* Parse the quantization tables (DQT) */
int parseDqt(uint8_t **ptr)
{
	uint16_t marker = swapBytes(*(uint16_t *)*ptr);
	int index, i;
	uint16_t length;
	uint8_t pqtq;

	*ptr += 2;
	if (marker == DQT_MARKER) {
		LOGD("Found DQT table!\n");
	} else {
		LOGD("Second DQT marker not found\r");
		return 0;
	}

	length = swapBytes(*(uint16_t *)*ptr);
	LOGD("Quantization table length is %u\n", length);

	*ptr += 2;
	// Figure out precision, as well as destination (whether Y or Cb/Cr)
	// TODO(pmalani): Assuming only 8 bit precision.

	do {
		pqtq = **ptr;

		if (!(pqtq & 0xF)) {
			LOGD("Found DQT for Y\n");
			index = 0;
		} else {
			LOGD("Found DQT for Cb/Cr\n");
			index = 1;
		}

		if (pqtq >> 4) {
			LOGE("Precision greater than 8 bits not supported\b");
			return -1;
		}

		jInfo->dqt[index].lq = length;
		jInfo->dqt[index].pq = pqtq >> 4;
		jInfo->dqt[index].tq = index;
		(*ptr)++;

		for (i = 0; i < 64; i++, (*ptr)++)
			jInfo->dqt[index].el[i] = **ptr;

		LOGD("The DQT table found is \n");
		for (i = 0; i < 64; i++)  {
			LOGD("%02u ", jInfo->dqt[index].el[i]);
			if (!((i + 1) % 8))
				LOGD("\n");
		}

		// Check for the second Quantization table if present
		pqtq = **ptr;
		if (pqtq == 0 || pqtq == 1) {
			jInfo->one_dqt = false;
			continue;
		} else {
			LOGD("There is no other quantization table\n");
			jInfo->one_dqt = true;
			break;
		}
	} while (jInfo->one_dqt == false);
	return 0;
}

/*
 * FUNCTION parseHeader
 *
 * Could endian-ness be a problem on Pi? need to figure out
 *
 * Parse the header to get initial metadata about image.
 */
int parseHeader()
{

	uint8_t *cur_ptr;
	uint8_t jump_len;
	uint16_t new_app0;
	// Cast to a jfif_header, and then parse.
	jfif_header *hdr = (jfif_header *)bArray;

	jInfo->hdr.soi = swapBytes(hdr->soi);
	if (jInfo->hdr.soi != SOI_MARKER)
		return -1;

	jInfo->hdr.app0 = swapBytes(hdr->app0);
	if (jInfo->hdr.app0 != 0xffe0)
		return -1;

	jInfo->hdr.app0_len = swapBytes(hdr->app0_len);
	LOGD("The APP0 segment length is %u\n", jInfo->hdr.app0_len);

	memcpy(jInfo->hdr.id_str, hdr->id_str, sizeof(hdr->id_str) /
			sizeof(hdr->id_str[0]));
	if (strcmp((char *)jInfo->hdr.id_str, "JFIF"))
		return -1;

	jInfo->hdr.version = hdr->version;
	LOGD("The version is %04x\n", jInfo->hdr.version);

	jInfo->hdr.x_res = swapBytes(hdr->x_res);
	LOGD("The X resolution is %u\n", jInfo->hdr.x_res);

	jInfo->hdr.y_res = swapBytes(hdr->y_res);
	LOGD("The y resolution is %u\n", jInfo->hdr.y_res);

	jInfo->hdr.units= hdr->units;
	LOGD("The units is %u\n", jInfo->hdr.units);

	jInfo->hdr.x_pixel = hdr->x_pixel;
	LOGD("The thumbnail X size is %u\n", jInfo->hdr.x_pixel);

	jInfo->hdr.y_pixel = hdr->y_pixel;
	LOGD("The thumbnail Y size is %u\n", jInfo->hdr.y_pixel);

	cur_ptr = &bArray[4 + jInfo->hdr.app0_len];

	// Keep skipping over the useless extension fields
	new_app0 = swapBytes(*(uint16_t *)cur_ptr);
	while (new_app0 == 0xFFEC || new_app0 == 0xFFED ||
			new_app0 == 0xFFEE || new_app0 == 0xFFEF) {
		LOGD("Skipping one extension field\n");
		cur_ptr += 2;
		cur_ptr += swapBytes(*(uint16_t *)cur_ptr);
		new_app0 = swapBytes(*(uint16_t *)cur_ptr);
	}

	if (parseDqt(&cur_ptr)) {
		LOGE("Error parsing QT\n");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		LOGE("Invalid number of arguments. Usage:\n"
				"./main --file <filename>\n");
		return -1;
	}

	if (loadFile(argv[2])) {
		LOGE("Error loading file\n");
		return -1;
	}

	jInfo = malloc(sizeof(jfif_info));

	if (parseHeader()) {
		LOGE("Error parsing Image header\n");
		free(jInfo);
		free(bArray);
		return -1;
	}

	free(jInfo);
	free(bArray);
	return 0;
}

