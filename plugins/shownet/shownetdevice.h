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
 * shownetdevice.h
 * Interface for the shownet device
 * Copyright (C) 2005  Simon Newton
 */

#ifndef SHOWNETDEVICE_H
#define SHOWNETDEVICE_H

#include <llad/device.h>
#include <llad/fdlistener.h>

#include <shownet/shownet.h>

#include "common.h"

class ShowNetDevice : public Device, public FDListener {

	public:
		ShowNetDevice(Plugin *owner, const string &name, class Preferences *prefs) ;
		~ShowNetDevice() ;

		int start() ;
		int stop() ;
		shownet_node get_node() const;
		int get_sd() const ;
		int fd_action() ;
		int save_config() const ;
		int configure(void *req, int len) ;

	private:
		class Preferences *m_prefs ;
		shownet_node m_node ;
		bool m_enabled ;
};

#endif
