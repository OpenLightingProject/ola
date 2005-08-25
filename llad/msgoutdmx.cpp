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
 * llad.hpp
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#include "messages/msgoutdmx.h" 

#include <string.h>

/*
 * create a new dmx out msg
 */
MsgOutDmx::MsgOutDmx(int type, struct sockaddr_in to) : MsgOut(type, to) {
	data.op = LLA_MSG_DMX_DATA ;
}

int MsgOutDmx::set_uni(uint8_t uni) {
	data.uni = uni ;
	return 0;
}

int MsgOutDmx::set_dmx(uint8_t *dmx, int length) {
	data.len = length<512 ? length : 512 ;
	memcpy(data.data, dmx, data.len) ;

	return 0;
}

int MsgOutDmx::send(Network *net) {
	return net->send_msg((uint8_t*) &data, sizeof(data), to ) ;
}
