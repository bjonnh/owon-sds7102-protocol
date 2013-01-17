/*
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
#include <errno.h>

#include <usb.h>
#include "usb.h"
#include "owon.h"

struct owon_start_command {
	const char *start;
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
	unsigned int bitmap; // 0 for waveform, 1 for bitmap
};

void owon_usb_init() {
    usb_init();
    usb_set_debug(USB_DEBUG);
}

size_t owon_usb_get_device_count() {
	size_t count = 0;

	usb_find_busses();
	usb_find_devices();

	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == OWON_USB_VENDOR_ID &&
				dev->descriptor.idProduct == OWON_USB_PRODUCT_ID) {
					count++;
			}
		}
	}

	return count;
}

struct usb_device *owon_usb_get_device(int dnum) {
	usb_find_busses();
	usb_find_devices();

	struct usb_bus *bus;
	struct usb_device *dev;

	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == OWON_USB_VENDOR_ID &&
				dev->descriptor.idProduct == OWON_USB_PRODUCT_ID) {
					if (dnum == 0)
						return dev;
					else
						dnum--;
			}
		}
	}

	return NULL;
}

struct usb_dev_handle *owon_usb_open(struct usb_device *dev) {
	struct usb_dev_handle *dev_handle = usb_open(dev);
	int ret;
	ret = usb_set_configuration(dev_handle, OWON_USB_CONFIGURATION);
	ret |= usb_claim_interface(dev_handle, OWON_USB_INTERFACE);
	if (0 > ret) {
		return NULL;
	}
	usb_reset(dev_handle);
	return dev_handle;
}

struct usb_dev_handle *owon_usb_easy_open(int dnum) {
	owon_usb_init();

	struct usb_device *dev = owon_usb_get_device(dnum);
	if (NULL == dev) {
		fprintf(stderr, "No devices found\n");
		exit(EXIT_FAILURE);
	}

	struct usb_dev_handle *dev_handle = owon_usb_open(dev);
	if (NULL == dev_handle) {
		fprintf(stderr, "Unable to open device\n");
		exit(EXIT_FAILURE);
	}

	return dev_handle;
}

int owon_usb_read(struct usb_dev_handle *dev_handle, char **buffer,
		  enum owon_start_command_type type) {
	struct owon_start_command *cmd;
	uint32_t allocated = 0, downloaded = 0;
	
	if (type >= DUMP_COUNT)
		return -1;

	cmd = &commands[type];

	// Send the START command.
	int ret;
	ret = usb_bulk_write(dev_handle, 
		OWON_USB_ENDPOINT_OUT, 
		cmd->start, 
		strlen(cmd->start), 
		OWON_USB_TRANSFER_TIMEOUT);
	if (strlen(cmd->start) != ret) {
		return OWON_ERROR_USB;
	}

	// Get the response back.
	struct owon_start_response start_response;
	ret = usb_bulk_read(dev_handle, 
		OWON_USB_ENDPOINT_IN, 
		(char *)&start_response, 
		cmd->response_length, 
		OWON_USB_TRANSFER_TIMEOUT);
	if (cmd->response_length != ret) {
		fprintf(stderr, "error usb: ret = %i\n", ret);
		return OWON_ERROR_USB;
	}

	// Allocate enough memory to hold the data from the ocilloscope.
	*buffer = malloc(start_response.length);
	if (NULL == *buffer) {
		return OWON_ERROR_MEMORY;
	}
	allocated = start_response.length;

	// Read data from the ocilloscope.
	downloaded = 0;
	do {
		if (downloaded + OWON_USB_READ_SIZE > allocated) {
			allocated += OWON_USB_REALLOC_INCREMENT;
			*buffer = realloc(*buffer, sizeof(uint8_t) * allocated);
			if (*buffer == NULL)
				return -ENOMEM;
		}

		ret = usb_bulk_read(dev_handle, OWON_USB_ENDPOINT_IN, *buffer + downloaded,
				    0xffff, OWON_USB_TRANSFER_TIMEOUT);

		if (ret > 0)
			downloaded += ret;
	} while (ret > 0);

	return downloaded; 
}

void owon_usb_close(struct usb_dev_handle *dev_handle) {
	usb_clear_halt(dev_handle, OWON_USB_ENDPOINT_IN);
	usb_clear_halt(dev_handle, OWON_USB_ENDPOINT_OUT);
	usb_release_interface(dev_handle, OWON_USB_INTERFACE);
	usb_close(dev_handle);
}

