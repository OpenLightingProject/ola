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
 * universe.hpp
 * Header file for the Universe class
 * Copyright (C) 2005  Simon Newton
 *
 */


#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <stdint.h>
#include <lla/port.h>
#include <vector>

#define DMX_LENGTH 512

using namespace std;

class Universe {

	public:
		~Universe() ;
		int 	add_port(Port *prt) ;
		int 	remove_port(Port *prt) ;
		int 	get_num_ports() const ;
	
		// int	add_client(Client *cli) ;
		// int 	remove_client(Client *cli) ;
		
		int set_dmx(uint8_t *dmx, int length) ;
		int get_dmx(uint8_t *dmx, int length) ;
		int get_uid() ;
		int port_data_changed(Port *prt) ;

		static Universe *get_universe(int uid) ;
		static Universe *get_universe_or_create(int uid) ;
		static int clean_up();

	protected :
		Universe(int uid) ;
		
	private:
		 
		int uid;									// universe address
//		merge_mode mm;								// merge mode
		vector<Port*> ports_vect ;		// ports assigned to the universe
//		vector<Clients *clients[MAX_CLIENTS_PER_UNIVERSE]	// clients listening to this universe
		uint8_t	data[DMX_LENGTH] ;					// buffer for this universe
		int length ;
		static vector<Universe*> uni_vect ;
		int update_dependants() ;
} ;


#endif
