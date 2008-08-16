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
 * port.h
 * Header file for the Port class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef PORT_H
#define PORT_H

#include <stdint.h>

class Device;
class Universe;

class Port {

  public:
    Port(Device *parent, int id);
    virtual ~Port() {};

    Device *get_device() const                { return m_parent; }
    int get_id() const                        { return m_pid; }
    virtual int set_universe(Universe *uni)   { m_universe = uni; return 0; }
    virtual Universe *get_universe() const    { return m_universe; }
    int dmx_changed();

    // subclasses must implement these
    virtual int write(uint8_t *data, unsigned int length) = 0;
    virtual int read(uint8_t *data, unsigned int length) = 0;

    // indicate our ports capability
    // default is read/write
    virtual int can_read() const  { return 1; }
    virtual int can_write() const { return 1; }

    // possible rdm functions here

  private:
    Port(const Port&);
    Port& operator=(const Port&);

    int m_pid;
    Universe *m_universe; // universe this port belongs to
    Device *m_parent;     // pointer to the device this port belongs to
};

#endif
