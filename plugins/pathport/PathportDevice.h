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
 * pathportdevice.h
 * Interface for the pathport device
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef PATHPORTDEVICE_H
#define PATHPORTDEVICE_H

#include <map>

#include <llad/device.h>
#include <llad/fdlistener.h>

#include <pathport/pathport.h>

#include "common.h"

class PathportDevice : public Device, public FDListener {

	public:
		PathportDevice(Plugin *owner, const string &name, class Preferences *prefs) ;
		~PathportDevice() ;

		int start() ;
		int stop() ;
		pathport_node get_node() const;
		int get_sd(unsigned int i) const ;
		int fd_action() ;
		int save_config() const ;
		int configure(void *req, int len) ;
        int port_map(class Universe *uni, class PathportPort *prt);
        class PathportPort *get_port_from_uni(int uni);

	private:
		class Preferences *m_prefs ;
		pathport_node m_node ;
	 	bool m_enabled ;
		map<int, class PathportPort *> m_portmap;
};

#endif
