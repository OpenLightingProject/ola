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
 * llad.h
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#ifndef LLAD_H
#define LLAD_H 


#include <lla/port.h>
#include <lla/messages.h>
#include "devicemanager.h"
#include "pluginloader.h" 
#include "network.h"

class Llad {

	public :
		Llad() ;
		~Llad() ;
		int init() ;
		int run() ;
		void terminate() ;

	private :
		int handle_syn(lla_msg *msg) ;
		int handle_fin(lla_msg *msg) ;
		int handle_msg(lla_msg *msg) ;
//		int	handle_info_request(Msg *msg) ;
		int handle_patch(lla_msg *msg) ;
//		int handle_register(Msg *msg) ;
		int handle_dmx_data(lla_msg *msg) ;
//		int handle_read_request(Msg *msg) ;

		int unpatch_port(Port *prt) ;
		int patch_port(Port *prt, Universe *uni) ;
		
		bool term ;
		int sd ;

		DeviceManager *dm ;
		PluginLoader *pm ;
		Network *net ;
		PluginAdaptor *pa;
};

#endif
