/*
 * Owon-dump, dumping data from Owon oscilloscopes
 * Copyright (c) 2012, 2013, 2014 Jonathan Bisson <bjonnh-owon@bjonnh.net>
 *                    Martin Peres <martin.peres@free.fr>
 * 
 * Based on:
 * owon-utils - a set of programs to use with OWON Oscilloscopes
 * Copyright (c) 2012  Levi Larsen <levi.larsen@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

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
	printf("usage: %s [-m (bmp|bin|memdepth|debugtxt)] [-p (raw|csv)] [-f output_file]", argv[0]);
	exit(EXIT_FAILURE);
}

/*void list_devices()
{
	size_t count, i;

	owon_usb_init();
	count = owon_usb_get_device_count();

	printf("Owon-dump: %u devices\n", count);

	exit(EXIT_SUCCESS);
	}*/

int parse_cli(int argc, char **argv, struct owon_dump_params *params)
{
	char c;

	params->dnum = 0;
	params->mode = DUMP_BIN;
	params->output = DUMP_OUTPUT_RAW;
	params->filename = NULL;

	while ((c = getopt (argc, argv, "m:o:f:")) != -1) {
		switch (c) {
/*			case 'd':
				sscanf(optarg, "%d", &params->dnum);
				break;*/
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
/*			case 'l':
				list_devices();
				break;*/
			default:
				usage(argc, argv);
				break;
		}
	}
	
	return 0;
}

int output_raw(FILE *fp, const char *buffer, long length)
{
	// Write data out
	fwrite(buffer, sizeof(char), length, fp);
}

int output_csv(FILE *fp, const char *buffer, long length)
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

	
	unsigned char *buffer;
	long length = -1;
	
	struct libusb_device_handle *dev_handle = owon_usb_easy_open(params.dnum);
	if (!dev_handle) {
		fprintf(stderr,"USB: Impossible to connect to device.\n");
		return 2;
	}

	length = owon_usb_read(dev_handle, &buffer, params.mode);
	owon_usb_close(dev_handle);

	if (0 >= length) {
		libusb_clear_halt(dev_handle,OWON_USB_ENDPOINT_IN);
		libusb_clear_halt(dev_handle,OWON_USB_ENDPOINT_OUT);
		libusb_reset_device(dev_handle);
		fprintf(stderr, "Error reading from device: %li\n", length);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"Writing file of length %d\n",length);
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
