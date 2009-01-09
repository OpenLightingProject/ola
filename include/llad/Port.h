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
 * Port.h
 * Header file for the Port class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef PORT_H
#define PORT_H

#include <stdint.h>

namespace lla {

class AbstractDevice;
class Universe;

class AbstractPort {
  public:
    AbstractPort() {}
    virtual ~AbstractPort() {};

    virtual AbstractDevice *GetDevice() const = 0;
    virtual int PortId() const = 0;
    virtual int SetUniverse(Universe *universe) = 0;
    virtual Universe *GetUniverse() const = 0;
    virtual int DmxChanged() = 0;

    // read/write dmx data to this port
    virtual int WriteDMX(uint8_t *data, unsigned int length) = 0;
    virtual int ReadDMX(uint8_t *data, unsigned int length) = 0;

    // indicate our port's capability
    virtual bool CanRead() const = 0;
    virtual bool CanWrite() const = 0;
};


class Port: public AbstractPort {
  public:
    Port(AbstractDevice *parent, int id);
    virtual ~Port() {};

    AbstractDevice *GetDevice() const               { return m_parent; }
    int PortId() const                      { return m_port_id; }
    int SetUniverse(Universe *uni)  { m_universe = uni; return 0; }
    Universe *GetUniverse() const   { return m_universe; }
    int DmxChanged();

    // default is read/write
    virtual bool CanRead()  const { return true; }
    virtual bool CanWrite() const { return true; }

  private:
    Port(const Port&);
    Port& operator=(const Port&);

    int m_port_id;
    Universe *m_universe; // universe this port belongs to
    AbstractDevice *m_parent;     // pointer to the device this port belongs to
};


} //lla
#endif
