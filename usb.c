/*
 * The usb part of owon-dump
 * Copyright (c) 2012, 2013, 2014 Jonathan Bisson <bjonnh-owon@bjonnh.net>
 *                    Martin Peres <>
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "usb.h"
#include "owon.h"

struct owon_start_command {
	char *start;
	uint8_t response_length;
} commands[] = {
	{ "STARTBMP", 12 },
	{ "STARTBIN", 12 },
	{ "STARTMEMDEPTH", 12 },
	{ "STARTDEBUGTXT", 4 }
};

struct owon_start_response {
	unsigned int length;
	unsigned int unknown;
	unsigned int flag; // 0 for waveform, 1 for bitmap, 128 if multipart
};

struct libusb_context *ctx = NULL;

void owon_usb_init() {
	libusb_init(&ctx);
	libusb_set_debug(ctx, USB_DEBUG);
}

size_t owon_usb_get_device_count() {
	size_t count = 0;

	struct libusb_device **list;
        struct libusb_device *found = NULL;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	ssize_t i = 0;
	int err = 0;
	if (cnt < 0)
		fprintf(stderr,"Error getting the device count\n");
		return -1;

	for (i = 0; i < cnt; i++) {
		struct libusb_device *device = list[i];
		err = owon_usb_is_managed(device);
		if (err>0)
			count++;
		if (err<0) {
			fprint(stderr,"Failed to list devices CODE=%d\n",err);
			libusb_free_device_list(list, 1);
			return -1;
		}
		
	}
	libusb_free_device_list(list, 1);
	return count;
}


// Returns 1 if the device is managed
// TODO: Add other Product_ID (testers needed)
int owon_usb_is_managed(void *device) {
	struct libusb_device_descriptor desc;
	struct libusb_device_handle *handle = NULL;
	
	int ret=0,err=0;

	ret = libusb_get_device_descriptor(device, &desc);
	if (ret < 0) 
		return -1;
	err = libusb_open(device,&handle);
	if (err < 0) {
		libusb_close(handle);
		return -2;
	}
	if (desc.idVendor == OWON_USB_VENDOR_ID && desc.idProduct == OWON_USB_PRODUCT_ID) {
		libusb_close(handle);
		return 1;
	}
	libusb_close(handle);
	return 0;
}

struct libusb_device_handle *owon_usb_get_device(int dnum) {
	size_t count = 0;

	struct libusb_device **list;
        struct libusb_device *found = NULL;
	struct libusb_device_handle *dev_handle = NULL;
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	ssize_t i = 0;
	int err = 0;
	if (cnt < 0) {
		fprintf(stderr,"Error getting the device count\n");
		return 0;
	}
	
	for (i = 0; i < cnt; i++) {
		struct libusb_device *device = list[i];
		if (owon_usb_is_managed(device)) // TODO: Manage multiple oscilloscopes on the same computer
			found = device;
	}
	libusb_free_device_list(list, 1);
	
	if (NULL != found) {
		err = libusb_open(found,&dev_handle);
		if (err != 0) {
			return NULL;
		}
	} else {
		fprintf(stderr, "Device not found\n");
		return NULL;
	}
	return dev_handle;
}

struct libusb_device_handle *owon_usb_open(struct libusb_device_handle *dev_handle) {
	int ret=0;
	int cfg;
	ret = libusb_get_configuration(dev_handle, &cfg);
	if (cfg != OWON_USB_CONFIGURATION)
		ret = libusb_set_configuration(dev_handle, OWON_USB_CONFIGURATION);
	ret |= libusb_claim_interface(dev_handle, OWON_USB_INTERFACE);

	if (0 > ret) {
		fprintf(stderr,"USB_Configuration error\n");
		return NULL;
	}

	return dev_handle;
}

struct libusb_device_handle *owon_usb_easy_open(int dnum) {
	owon_usb_init();
	
	struct libusb_device_handle *dev_handle = owon_usb_get_device(dnum);
       
	if (NULL == dev_handle) {
		fprintf(stderr, "Unable to open device\n");
		return NULL;
	}
	dev_handle = owon_usb_open(dev_handle);

	return dev_handle;
}

int owon_get_response(struct owon_start_command *cmd, struct libusb_device_handle *dev_handle, struct owon_start_response *start_response)
{
	int ret=-255;
	uint32_t transferred = 0;
	uint8_t tries=3;
	char start_response2[0x0c];
	do {

		ret = libusb_bulk_transfer(dev_handle, 
					   OWON_USB_ENDPOINT_IN, 
					   (char *) start_response2, 
					   sizeof(start_response2), &transferred,
					   OWON_USB_TRANSFER_TIMEOUT);
		fprintf(stderr,"Try %d ret=%d\n",3-tries,ret);
	} while (tries-->0 && ret<0);
	fprintf(stderr,"Get_response code=%d  transferred=%d size=%d\n",ret,transferred,sizeof(start_response2));

	if (ret<0)
		return -1;

	if (cmd->response_length != transferred && transferred!=0) {
		fprintf(stderr, "error usb: ret = %i resplength=%d, transferred=%d\n", ret,cmd->response_length,transferred);
		return -1;
	}
	memcpy(start_response,&start_response2,sizeof(start_response2));
	return 0;
}

int owon_usb_read(struct libusb_device_handle *dev_handle, unsigned char **buffer,
		  enum owon_start_command_type type) {
	struct owon_start_command *cmd;
	struct owon_start_response start_response;
	int multipart = 0;
	uint32_t allocated = 0, downloaded = 0;
	uint32_t transferred = 0;
	if (type >= DUMP_COUNT)
		return -1;

	cmd = &commands[type];

	// Send the START command.
	int ret;

	ret = libusb_bulk_transfer(dev_handle,OWON_USB_ENDPOINT_OUT,cmd->start,strlen(cmd->start),&transferred,OWON_USB_TRANSFER_TIMEOUT);    
	if (strlen(cmd->start) != transferred || ret!=0) {
		fprintf(stderr,"Command send error ret=%d\n",ret);
		return OWON_ERROR_USB;
	}
	
	// Get the response back.

	do {
		ret = owon_get_response(cmd, dev_handle, &start_response);
	
		fprintf(stderr,"resp: ret=%d",ret);
		if (ret>=0)
			fprintf(stderr," %d %d %d ", start_response.length,start_response.unknown,start_response.flag);
		fprintf(stderr,"\n");
		if (ret == -1) {
			if (multipart==1 && start_response.length==0) {
				return downloaded;
			}
			if (allocated==downloaded) {
				return downloaded;
			}
			return -1;
		}
		if (start_response.flag > 128) {
			fprintf(stderr,"Multipart\n");
			multipart=1;
		} else {
			multipart=0;
		}

		// Allocate enough memory to hold the data from the ocilloscope.
		if (allocated == 0) {
			fprintf(stderr,"Allocating %d\n",start_response.length);
			*buffer = malloc(start_response.length);
			if (NULL == *buffer) {
				fprintf(stderr,"Error allocating %d\n",start_response.length);
				return OWON_ERROR_MEMORY;
			}
			allocated = start_response.length;
		} else {
			fprintf(stderr,"Reallocating %d\n",start_response.length+allocated);
			*buffer = realloc(*buffer, start_response.length + allocated);
			if (*buffer == NULL) {
				fprintf(stderr,"Error reallocating %d\n",start_response.length+allocated);
				return OWON_ERROR_MEMORY;
			}
			allocated += start_response.length;
		}
     
	// Read data from the ocilloscope.
		int forloop;
		forloop=start_response.length;
	do {
		int tries=3;
		ret=-255;
		do {
			ret = libusb_bulk_transfer(dev_handle, OWON_USB_ENDPOINT_IN, *buffer + downloaded,
						   131072,&transferred, 50000);
			if (ret<0) {
				fprintf(stderr,"Try %d ret=%d transf=%d\n",3-tries,ret,transferred);
				usleep(100);
			}
		} while (tries-->0 && ret<0);

		forloop -= transferred;
		downloaded += transferred;

		if (ret==-1)
			return -1;
		fprintf(stderr,"%d/%d %d %% (rest=%d) ret=%d transferred=%d\n",downloaded,allocated,100*downloaded/allocated,forloop,ret,transferred);
	} while (allocated > downloaded);
//	fprintf(stderr,"%d/%d %d %% (rest=%d) ret=%d\n",downloaded,allocated,100*downloaded/allocated,forloop,ret);
	} while (multipart != 0);
	fprintf(stderr,"Downloaded: %d\n",downloaded);
	return downloaded; 
}

void owon_usb_close(struct libusb_device_handle *dev_handle) {
	libusb_release_interface(dev_handle, OWON_USB_INTERFACE);
	libusb_close(dev_handle);
	libusb_exit(ctx);
}

