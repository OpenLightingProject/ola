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
 * Interface for the llad class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef LLAD_H
#define LLAD_H 


#include <lla/port.h>
#include <lla/messages.h>
#include <lla/preferences.h>
#include <lla/plugin_id.h>

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
		void reload_plugins() ;

	private :
		Llad(const Llad&);
		Llad& operator=(const Llad&);

		int handle_syn(lla_msg *msg) ;
		int handle_fin(lla_msg *msg) ;
		int handle_msg(lla_msg *msg) ;
		int handle_read_request(lla_msg *msg) ;
		int	handle_plugin_info_request(lla_msg *msg) ;
		int	handle_plugin_desc_request(lla_msg *msg) ;
		int	handle_device_info_request(lla_msg *msg) ;
		int	handle_port_info_request(lla_msg *msg) ;
		int	handle_universe_info_request(lla_msg *msg) ;
		int handle_patch(lla_msg *msg) ;
		int handle_uni_name(lla_msg *msg) ;
		int handle_register(lla_msg *msg) ;
		int handle_dmx_data(lla_msg *msg) ;
//		int handle_read_request(Msg *msg) ;
		int handle_device_config_request(lla_msg *msg) ;
		
		int send_dmx(Universe *uni, struct sockaddr_in dst) ;
		int send_plugin_info(struct sockaddr_in dst) ;
		int send_plugin_desc(struct sockaddr_in dst, Plugin *plug, int pid) ;
		int send_device_info(struct sockaddr_in dst, lla_plugin_id filter) ;
		int send_universe_info(struct sockaddr_in dst) ;
		int send_port_info(struct sockaddr_in dst, Device *dev, int devid) ;

		int unpatch_port(Port *prt) ;
		int _reload_plugins() ;

		bool m_term ;
		bool m_reload_plugins ;

		DeviceManager *dm ;
		PluginLoader *pm ;
		Network *net ;
		PluginAdaptor *pa;
		Preferences m_uni_names;
};

#endif
