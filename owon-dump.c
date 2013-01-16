#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <usb.h>
#include "usb.h"
#include "parse.h"

struct owon_dump_params {
	uint8_t dnum;
	enum owon_start_command_type mode;
	enum owon_output_type output;
	char *filename;
};

void usage(int argc, char **argv)
{
	printf("usage: %s [-d device_number] [-m (bmp|bin|memdepth|debugtxt)] [-p (raw|csv)] [-f output_file]", argv[0]);
	exit(EXIT_FAILURE);
}

void list_devices()
{
	size_t count, i;

	owon_usb_init();
	count = owon_usb_get_device_count();

	printf("Owon-dump: %u devices\n", count);
	for (i = 0; i < count; i++) {
		struct usb_device *dev = owon_usb_get_device(i);
		printf("	%u: bus %s device %u\n", i, dev->bus->dirname, dev->devnum);
	}
	
	exit(EXIT_SUCCESS);
}

int parse_cli(int argc, char **argv, struct owon_dump_params *params)
{
	char c;

	params->dnum = 0;
	params->mode = DUMP_BIN;
	params->output = DUMP_OUTPUT_RAW;
	params->filename = NULL;

	while ((c = getopt (argc, argv, "d:m:o:f:l")) != -1) {
		switch (c) {
			case 'd':
				sscanf(optarg, "%d", &params->dnum);
				break;
			case 'm':
				if (strcasecmp(optarg, "bmp") == 0)
					params->mode = DUMP_BMP;
				else if (strcasecmp(optarg, "bin") == 0)
					params->mode = DUMP_BIN;
				else if (strcasecmp(optarg, "memdepth") == 0)
					params->mode = DUMP_MEMDEPTH;
				else if (strcasecmp(optarg, "debugtxt") == 0)
					params->mode = DUMP_DEBUGTXT;
				else
					return 1;
				break;
			case 'o':
				if (strcasecmp(optarg, "raw") == 0)
					params->output = DUMP_OUTPUT_RAW;
				else if (strcasecmp(optarg, "csv") == 0)
					params->output = DUMP_OUTPUT_CSV;
				else
					return 1;
				break;
			case 'f':
				params->filename = strdup(optarg);
				break;
			case 'l':
				list_devices();
				break;
			default:
				usage(argc, argv);
				break;
		}
	}
	
	return 0;
}

int output_raw(FILE *fp, char *buffer, long length)
{
	// Write data out
	fwrite(buffer, sizeof(char), length, fp);
}

int output_csv(FILE *fp, char *buffer, long length)
{
	HEADER_st header;

	int ret = owon_parse(buffer, length, &header);
	if (ret < 0)
		return ret;

	owon_output_csv(&header, fp);
}

int main (int argc, char **argv)
{
	struct owon_dump_params params;

	if (parse_cli(argc, argv, &params))
		usage(argc, argv);

	struct usb_dev_handle *dev_handle = owon_usb_easy_open(params.dnum);
	if (!dev_handle)
		return 2;
	
	char *buffer;
	long length = 0;
	length = owon_usb_read(dev_handle, &buffer, params.mode);
	owon_usb_close(dev_handle);
	if (0 > length) {
		fprintf(stderr, "Error reading from device: %li\n", length);
		exit(EXIT_FAILURE);
	}
	
	// Get file pointer to file or stdout.
	FILE *fp;
	if (NULL == params.filename) {
		fp = stdout;
	} else {
		fp = fopen(params.filename, "wb");
		if (NULL == fp) {
			fprintf(stderr, "Unable to open %s\n", params.filename);
			exit(EXIT_FAILURE);
		}
	}

	switch (params.output) {
	case DUMP_OUTPUT_RAW:
		output_raw(fp, buffer, length);
		break;
	case DUMP_OUTPUT_CSV:
		output_csv(fp, buffer, length);
		break;
	}
	
	free(buffer);

	// Only close fp if it's an actually file (don't close stdout).
	if (NULL != params.filename) {
	fclose(fp);
	}

	return 0;
}