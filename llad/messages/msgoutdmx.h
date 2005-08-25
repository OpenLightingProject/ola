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


#include "msgout.h"
#include <lla/messages.h>
#include "network.h"

class MsgOutDmx : public MsgOut {

	public :
		MsgOutDmx(struct sockaddr_in to) ;
		int send(Network *net) ;
		int set_uni(uint8_t uni) ;
		int set_dmx(uint8_t *dmx, int length) ;

	protected :
		struct sockaddr_in to;
		int type ;
		lla_msg_dmx_data data ;
};
