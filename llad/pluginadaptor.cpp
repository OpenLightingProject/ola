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
 * pluginadaptor.cpp
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 *
 * Provides a wrapper for the DeviceManager and Network objects so that the plugins
 * can register devices and file handles for events
 * 
 */

#include <lla/pluginadaptor.h>
#include "devicemanager.h"
#include "network.h"

/*
 * Create a new pluginadaptor
 *
 */
PluginAdaptor::PluginAdaptor(DeviceManager *dm, Network *net) {
	this->dm = dm ;
	this->net = net;
}

int PluginAdaptor::register_fd(int fd, PluginAdaptor::Direction dir, FDListener *listener) {
	Network::Direction ndir = dir==PluginAdaptor::READ ? Network::READ : Network::WRITE ;
	return net->register_fd(fd,ndir,listener) ;
}

int PluginAdaptor::unregister_fd(int fd, PluginAdaptor::Direction dir) {
	Network::Direction ndir = dir==PluginAdaptor::READ ? Network::READ : Network::WRITE ;
	return net->unregister_fd(fd,ndir) ;
}

int PluginAdaptor::register_device(Device *dev) {
	return dm->register_device(dev) ;
}

int PluginAdaptor::unregister_device(Device *dev) {
	return dm->unregister_device(dev) ;
}

