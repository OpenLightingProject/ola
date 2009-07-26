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
 * E131Device.h
 * Interface for the E1.31 device
 * Copyright (C) 2007 Simon Newton
 */

#ifndef E131DEVICE_H
#define E131DEVICE_H

#include <olad/device.h>
#include <olad/fdlistener.h>

class E131Device : public Device {

  public:
    E131Device(Plugin *owner, const string &name, class NetServer *ns, class Preferences *prefs) ;
    ~E131Device() ;

    int start() ;
    int stop() ;
    int fd_action() ;
    int save_config() const;
    class OlaDevConfMsg *configure(const uint8_t *req, int len) ;

  private:
    class Preferences *m_prefs;
    class NetServer *m_ns;
    class E131Node *m_node;
    class E131DmpLayer *m_layer;
    bool m_enabled ;

};

#endif
