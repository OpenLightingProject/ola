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
 * sandnetdevice.h
 * Interface for the sandnet device
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef SANDNETDEVICE_H
#define SANDNETDEVICE_H

#include <map>

#include <lla/device.h>

#include <lla/fdlistener.h>
#include <lla/timeoutlistener.h>

#include <sandnet/sandnet.h>

#include "common.h"

class SandNetDevice : public Device, public FDListener, public TimeoutListener {

	public:
		SandNetDevice(Plugin *owner, const char *name, class Preferences *prefs) ;
		~SandNetDevice() ;

		int start() ;
		int stop() ;
		sandnet_node get_node() const;
		int get_sd(int i) const ;
		int fd_action() ;
		int timeout_action();
		int save_config() ;
		int configure(void *req, int len) ;

		int port_map(class Universe *uni, class SandNetPort *prt);
		class SandNetPort *get_port(int uni);
	private:
		class Preferences *m_prefs ;
		sandnet_node m_node ;
		bool m_enabled ;
		map<int, class SandNetPort *> m_portmap;
};

#endif
