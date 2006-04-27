/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * usbproport.cpp
 * The Usb Prp plugin for lla
 * Copyright (C) 2006  Simon Newton
 */

#include "usbproport.h"
#include "usbprodevice.h"
#include <lla/universe.h>
#include <lla/logger.h>
#include <string.h>



int UsbProPort::can_read() {
	// even ports are input
	return ( (get_id()+1) % 2);
}

int UsbProPort::can_write() {
	// odd ports are output
	return ( get_id() % 2 );
}

/*
 * Write operation
 * 
 * @param	data	pointer to the dmx data
 * @param	length	the length of the data
 *
 * @return 	0 on success, non 0 on failure
 */
int UsbProPort::write(uint8_t *data, int length) {
	UsbProDevice *dev = (UsbProDevice*) get_device() ;
	int ret ;

	if( !can_write())
		return -1 ;
	
	// send to device
	return dev->send_dmx(data, length) ;
	
}

/*
 * Read operation
 *
 * @param 	data	buffer to read data into
 * @param 	length	length of data to read
 *
 * @return	the amount of data read
 */
int UsbProPort::read(uint8_t *data, int length) {
	uint8_t *dmx = NULL;
	int len ;
	UsbProDevice *dev = (UsbProDevice*) get_device() ;
	
//	printf("in read %i\n", can_read() ) ;
	if( !can_read()) 
		return -1 ;
	
	// get the device to copy into the buffer
	return dev->get_dmx(data, length) ;
	
}


