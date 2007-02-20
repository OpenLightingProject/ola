/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * LlaPort.h
 * Interface to the LLA Client Port class
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef LLA_CLIENT_PORT_H
#define LLA_CLIENT_PORT_H

using namespace std;

#include <string>

class LlaPort {

  public:
    enum PortCapability { LLA_PORT_CAP_IN, LLA_PORT_CAP_OUT};

    LlaPort(int id, PortCapability cap, int uni, int active);
    ~LlaPort() {};

    int get_id() { return m_id; }
    PortCapability get_capability() { return m_cap; }
    int get_uni() { return m_uni; }
    int is_active() { return m_active; }

  private:
    LlaPort(const LlaPort&);
    LlaPort operator=(const LlaPort&);

    int m_id;        // id of this port
    PortCapability m_cap;  // port capability
    int m_uni;        // universe
    int m_active;      // active
};
#endif
