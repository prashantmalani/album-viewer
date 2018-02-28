/*
 * main.c
 *
 * Main file for execution of the JPEG decoder
 *
 * Author: Prashant Malani <p.malani@gmail.com>
 * Date  : 04/20/2015
 *
 */
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "jpeg_dec.h"
#include "jpeg_util.h"
#include "jpeg_header.h"

/* Array which stores the raw JPEG bytes */
uint8_t *bArray;
/* Array which stores cleaned scan data, without app markers */
uint8_t *cleanArray;

int debug_level = LOG_DEBUG;

/*
 * Global data structure which will store all the file data,
 * after it has been parsed.
 */
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
	cleanArray = malloc(fsize * sizeof(uint8_t));
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
		LOGD("DQT marker not found\r");
		return -1;
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

int parseSof(uint8_t **ptr)
{
	uint16_t marker = swapBytes(*(uint16_t *)*ptr);
	uint16_t length;
	int i;

	*ptr += 2;
	if (marker == SOF_MARKER) {
		LOGD("Found SOF table!\n");
	} else {
		LOGD("SOF marker not found\r");
		return -1;
	}


	length = swapBytes(*(uint16_t *)*ptr);
	LOGD("SOF table length is %u\n", length);
	*ptr += 2;

	jInfo->sof.prec = **ptr;
	LOGD("The SOF sample precision is %u\n", jInfo->sof.prec);
	(*ptr)++;

	jInfo->sof.y = swapBytes(*(uint16_t *)*ptr);
	LOGD("SOF number of Y lines is %u\n", jInfo->sof.y);
	*ptr += 2;
	jInfo->sof.x = swapBytes(*(uint16_t *)*ptr);
	LOGD("SOF number of X lines is %u\n", jInfo->sof.x);
	*ptr += 2;

	// TODO: Need to free this when we're done.
	jInfo->r = (uint8_t **)malloc(jInfo->sof.y * sizeof(uint8_t *));
	jInfo->g = (uint8_t **)malloc(jInfo->sof.y * sizeof(uint8_t *));
	jInfo->b = (uint8_t **)malloc(jInfo->sof.y * sizeof(uint8_t *));
	for (i = 0; i < jInfo->sof.y; i++) {
		jInfo->r[i] = (uint8_t *)malloc(jInfo->sof.x *
						sizeof(uint8_t));
		jInfo->g[i] = (uint8_t *)malloc(jInfo->sof.x *
						sizeof(uint8_t));
		jInfo->b[i] = (uint8_t *)malloc(jInfo->sof.x *
						sizeof(uint8_t));
	}

	jInfo->sof.num_f = **ptr;
	LOGD("The number of image components is %u\n", jInfo->sof.num_f);
	(*ptr)++;

	for (i = 0; i < jInfo->sof.num_f; i++) {
		jInfo->sof.comp[i].c = **ptr;
		(*ptr)++;
		jInfo->sof.comp[i].h = **ptr >> 4;
		jInfo->sof.comp[i].v = **ptr & 0xF;
		(*ptr)++;

		jInfo->sof.comp[i].qt_sel = **ptr;
		(*ptr)++;
		LOGD("Specs for comp=%d are c=%u, h=%u, v=%u, qt_sel=%u\n",
			i,
			jInfo->sof.comp[i].c,
			jInfo->sof.comp[i].h,
			jInfo->sof.comp[i].v,
			jInfo->sof.comp[i].qt_sel);
	}

	return 0;
}

int parseHuff(uint8_t **ptr)
{
	uint16_t marker = swapBytes(*(uint16_t *)*ptr);
	uint16_t length;
	int i, num_codes;
	uint8_t class_id;
	uint8_t cl, id;

	do {
		if (marker == HUF_MARKER) {
			LOGD("Found HUF table!\n");
		} else {
			LOGD("HUF marker not found\r");
			break;
		}

		// Advance the ptr only if we know it's a Huffman table.
		*ptr += 2;

		length = swapBytes(*(uint16_t *)*ptr);
		LOGD("Table length is %u\n", length);
		*ptr += 2;

continuous_tables:
		class_id = **ptr;
		(*ptr)++;

		cl = class_id >> 4;
		id = class_id & 0xF;

		LOGD("Table class = %u, id = %u\n", cl, id);

		memcpy(jInfo->huff[cl][id].l, *ptr, 16);
		*ptr += 16;

		num_codes = 0;
		// Need to malloc space to copy the actual code-values
		for (i = 0; i < 16; i++) {
			LOGD("Number of codes of length %d : %u\n", (i + 1),
					jInfo->huff[cl][id].l[i]);
			num_codes += jInfo->huff[cl][id].l[i];
		}

		jInfo->huff[cl][id].codes = malloc(sizeof(uint8_t) * num_codes);

		memcpy(jInfo->huff[cl][id].codes, *ptr, num_codes);
		*ptr += num_codes;

		//Now we need to construct the table
		genHuff(&(jInfo->huff[cl][id]));

		marker = swapBytes(*(uint16_t *)*ptr);
		// Here we make a decision whether each Huffman table is
		// preceded by the App marker or not. And alter the loop
		// accordingly.
		if ((marker & APP_MASK) != APP_MASK) {
			goto continuous_tables;
		}
	} while (true); // TODO(pmalani): Beware of endless loops

	return 0;
}

int parseSos(uint8_t **ptr)
{
	uint16_t marker = swapBytes(*(uint16_t *)*ptr);
	uint16_t length;
	uint8_t comp_sel, comp_ac_dc;
	int i;

	if (marker == SOS_MARKER) {
		LOGD("Found Start of Scan marker\n");
	} else {
		LOGE("Start of Scan not found\n");
		return -1;
	}

	*ptr += 2;
	length = swapBytes(*(uint16_t *)*ptr);
	LOGD("Start of scan header size is %u\n", length);
	*ptr += 2;

	jInfo->sos.num_c = **ptr;
	(*ptr)++;

	for (i = 0; i < jInfo->sos.num_c; i++) {
		comp_sel = **ptr;
		(*ptr)++;
		comp_ac_dc = **ptr;
		(*ptr)++;

		jInfo->sos.dc_sel[comp_sel] = comp_ac_dc >> 4;
		jInfo->sos.ac_sel[comp_sel] = comp_ac_dc & 0xF;

		LOGD("Coding tables comp=%u, dc=%u, ac=%u\n", comp_sel,
			jInfo->sos.dc_sel[comp_sel],
			jInfo->sos.ac_sel[comp_sel]);
	}

	// Just to ensure that we're decoding correctly, check that the
	// remaining bits are also what we think they should be.
	if (**ptr != 0) {
		LOGD("Start of spec selection byte incorrect!\n");
		return -1;
	}
	(*ptr)++;

	if (**ptr != 63) {
		LOGD("End of spec selection byte incorrect!\n");
		return -1;
	}
	(*ptr)++;

	if (**ptr != 0) {
		LOGD("Approx. info byte incorrect!\n");
		return -1;
	}
	(*ptr)++;

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	uint8_t *cur_ptr;
	uint16_t new_app0;

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

	if (parseHeader(bArray, cleanArray, &jInfo->hdr)) {
		LOGE("Error parsing Image header\n");
		ret = -1;
		goto ret_err;
	}

	// We don't care about the thumbnail image so we can skip it.
	cur_ptr = &bArray[4 + jInfo->hdr.app0_len];

	// Keep skipping over the extension fields.
	new_app0 = swapBytes(*(uint16_t *)cur_ptr);
	while (new_app0 == 0xFFEC || new_app0 == 0xFFED ||
			new_app0 == 0xFFEE || new_app0 == 0xFFEF) {
		LOGD("Skipping one extension field: %04x\n", new_app0);
		cur_ptr += 2;
		cur_ptr += swapBytes(*(uint16_t *)cur_ptr);
		new_app0 = swapBytes(*(uint16_t *)cur_ptr);
	}

	// From here on in we can actually refer only to cleanArray
	parseScanData(cleanArray, jInfo);

	writeToBmp(jInfo->r, jInfo->g, jInfo->b, jInfo->sof.x, jInfo->sof.y);

ret_err:
	free(jInfo);
	free(bArray);
	free(cleanArray);
	return ret;
}

