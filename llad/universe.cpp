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
 * universe.cpp
 * Defines the operations on the universe class
 * Copyright (C) 2005  Simon Newton
 *
 */

#include <lla/universe.h>
#include <lla/logger.h>

#include <iterator>
#include <string.h>
#include <stdio.h>

vector<Universe*> Universe::uni_vect ;

/*
 * Create a new universe
 *
 */
Universe::Universe(int uid) {
	int i;
	
	this->uid = uid ;
	
	// zero dmx buffer
	memset(data, 0x00, DMX_LENGTH) ;
	length = DMX_LENGTH ;

}


/*
 *
 *
 */
Universe::~Universe() {




}

/*
 * Add a port to this universe
 *
 *
 */
int Universe::add_port(Port *prt) {
	int i ;
	Universe *uni ;
	vector<Port*>::iterator it ;
	
	// unpatch if required
	uni = prt->get_universe()  ;

	if( uni != NULL) {
		Logger::instance()->log(Logger::DEBUG, "Port %p is bound to universe %d", prt, uni->get_uid()) ;
		uni->remove_port(prt) ;

		// we need to check here and destroy this universe if that was the last port
		// or do we? what if we had a client listening ?
	}

	// patch to this universe
	Logger::instance()->log(Logger::INFO, "Patched %d to universe %d",prt->get_id() , uid) ;
	ports_vect.push_back(prt) ;
	
	prt->set_universe(this) ;	

	return 0;
}

/*
 * Remove a port from this universe
 *
 *
 */
int Universe::remove_port(Port *prt) {
	int i ;
	vector<Port*>::iterator it;
	
	it = find(ports_vect.begin(),ports_vect.end(),prt); // first position

	if(it != ports_vect.end() ) {
		ports_vect.erase(it) ;
		prt->set_universe(NULL) ;
		Logger::instance()->log(Logger::DEBUG, "Port %p has been removed from uni %d", prt, get_uid()) ;
		
	} else {
		Logger::instance()->log(Logger::WARN, "Could not find port in universe") ;
		return -1 ;
	}

	return 0;
}


/*
 * Returns the number of ports assigned to this universe
 *
 */
int Universe::get_num_ports() const {
	return ports_vect.size() ;
}


/*
 * Set the dmx data for this universe
 *
 *
 */
int Universe::set_dmx(uint8_t *dmx, int len) {
	
	length = len < DMX_LENGTH ? len : DMX_LENGTH ;
	memcpy(data, dmx, length) ;

	return this->update_dependants() ;
}


/*
 * Set the dmx data for this universe
 *
 *
 */
int Universe::get_dmx(uint8_t *dmx, int length) {
	int len ;
	
	len = length < DMX_LENGTH ? length : DMX_LENGTH ;
	memcpy(dmx, data, len) ;

	return len ;
}


int Universe::get_uid() {
	return uid ;
}



/*
 * Call when a port that is part of this universe changes
 *
 *
 */
int Universe::port_data_changed(Port *prt) {
	
	// if the port is in the current list
	for(int i =0 ; i < ports_vect.size() ; i++) {
		if(ports_vect[i] == prt && prt->can_read() ) {
			// read the new data and update our dependants
			length = prt->read(data, DMX_LENGTH) ;

			update_dependants() ;
		}
	}
	return 0;
}


// Private Methods
//-----------------------------------------------------------------------------


/*
 * Called when the dmx data for this universe changes,
 * updates everyone who needs to know (bound ports and network clients)
 *
 */
int Universe::update_dependants() {
	int i ;

	// write to all ports assigned to this unviverse
	for(i=0 ; i < ports_vect.size() ; i++) {
		ports_vect[i]->write(data, length);
	}

	// write to all clients


	return 0;
}


// Class Methods
//-----------------------------------------------------------------------------

/*
 * lookup a universe from it's address, creates one if it does not exist
 *
 *
 */
Universe *Universe::get_universe(int uid) {
	int i ;

	for(i=0; i < uni_vect.size() ; i++) {
		if(uni_vect[i]->get_uid() == uid) 
			return uni_vect[i] ;
	}

	return NULL ;
}

/*
 *
 *
 *
 */
Universe *Universe::get_universe_or_create(int uid) {
	int i ;
	Universe *uni = get_universe(uid) ;

	if(uni == NULL) {
		uni = new Universe(uid) ;
		uni_vect.push_back(uni) ;
	}

	// this could still be NULL
	return uni ;
}

int Universe::clean_up() {
	for(int i=0; i < uni_vect.size() ; i++) {
		delete uni_vect[i] ;
	}
}



