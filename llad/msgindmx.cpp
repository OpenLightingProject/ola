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
 * dmxmsg.h
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#include "messages/msgindmx.h"
#include <string.h>


MsgInDmx::MsgInDmx(struct sockaddr_in *from, uint8_t *data, int len) : MsgIn(from) {
	
	memcpy(&this->data, data, len) ;

}

/*
 * Get the universe of this msg
 */
uint16_t MsgInDmx::get_uid() {
	return data.uni ;
}

/*
 * Get the universe of this msg
 */
int MsgInDmx::get_type() {
	return LLA_MSG_DMX_DATA ;
}


uint8_t *MsgInDmx::get_dmx() {
	return (uint8_t*) &data.data ;
}

int MsgInDmx::get_length() {
	return data.len ;
}
