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

/* Array which stores the raw JPEG bytes */
uint8_t *bArray;

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

	LOGI("End of execution\n");
	return 0;
}

