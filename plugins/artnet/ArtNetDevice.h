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
 * artnetdevice.h
 * Interface for the artnet device
 * Copyright (C) 2005  Simon Newton
 */

#ifndef ARTNETDEVICE_H
#define ARTNETDEVICE_H

#include <llad/device.h>
#include <llad/fdlistener.h>

#include <artnet/artnet.h>

class ArtNetDevice : public Device, public FDListener {

  public:
    ArtNetDevice(Plugin *owner, const string &name, class Preferences *prefs) ;
    ~ArtNetDevice() ;

    int start() ;
    int stop() ;
    artnet_node get_node() const;
    int get_sd() const ;
    int fd_action() ;
    int save_config() const;
    class LlaDevConfMsg *configure(const uint8_t *req, int len) ;

  private:
    class Preferences *m_prefs;
    artnet_node m_node ;
    bool m_enabled ;
    class ArtNetConfParser *m_parser;    // parser for config msgs

};

#endif
