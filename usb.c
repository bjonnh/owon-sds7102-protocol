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

#ifdef WIN32
#include <lusb0_usb.h>
#else
#include <usb.h>
#endif
#include "usb.h"
#include "owon.h"
#include "stdio.h"

void owon_usb_init() {
    usb_init();
    usb_set_debug(USB_DEBUG);
}

struct usb_device *owon_usb_get_device() {
    usb_find_busses();
    usb_find_devices();

    struct usb_bus *bus;
    struct usb_device *dev;

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == OWON_USB_VENDOR_ID &&
                    dev->descriptor.idProduct == OWON_USB_PRODUCT_ID) {
                return dev;
            }
        }
    }

    return NULL;
}

/* TODO
struct usb_device **owon_usb_get_devices() {
    return NULL;
}
*/

struct usb_dev_handle *owon_usb_open(struct usb_device *dev) {
    struct usb_dev_handle *dev_handle = usb_open(dev);
    int ret;
    ret = usb_set_configuration(dev_handle, OWON_USB_CONFIGURATION);
    ret = usb_claim_interface(dev_handle, OWON_USB_INTERFACE);
    //ret = usb_clear_halt(dev_handle, OWON_USB_ENDPOINT_IN);
    //ret = usb_clear_halt(dev_handle, OWON_USB_ENDPOINT_OUT);
    if (0 > ret) {
        return NULL;
    }
    return dev_handle;
}

int owon_usb_read(struct usb_dev_handle *dev_handle, char **buffer) {
    // Send the START command.
    int ret;
    ret = usb_bulk_write(dev_handle, 
            OWON_USB_ENDPOINT_OUT, 
            OWON_START_CMD, 
            OWON_START_CMD_LEN, 
            OWON_USB_TRANSFER_TIMEOUT);
    if (OWON_START_CMD_LEN != ret) {
        return OWON_ERROR_USB;
    }

    // Get the response back.
    struct owon_start_response start_response;
    ret = usb_bulk_read(dev_handle, 
            OWON_USB_ENDPOINT_IN, 
            (char *)&start_response, 
            OWON_START_RESPONSE_LEN, 
            OWON_USB_TRANSFER_TIMEOUT);
    if (OWON_START_RESPONSE_LEN != ret) {
        return OWON_ERROR_USB;
    }

    // Allocate enough memory to hold the data from the ocilloscope.
    *buffer = malloc(start_response.length);
    if (NULL == *buffer) {
        return OWON_ERROR_MEMORY;
    }
   
    // Read the data from the ocilloscope.
    ret = usb_bulk_read(dev_handle, 
            OWON_USB_ENDPOINT_IN, 
            *buffer, 
            start_response.length, 
            OWON_USB_TRANSFER_TIMEOUT);
    if (start_response.length != ret) {
        return OWON_ERROR_USB;
    }

    return start_response.length; 
}

void owon_usb_close(struct usb_dev_handle *dev_handle) {
    usb_release_interface(dev_handle, OWON_USB_INTERFACE);
    usb_close(dev_handle);
}

