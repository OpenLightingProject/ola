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
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORT_H_
#define INCLUDE_OLAD_PORT_H_

#include <string>
#include <ola/DmxBuffer.h>  // NOLINT
#include <olad/Device.h>  // NOLINT
#include <olad/Plugin.h>  // NOLINT
#include <olad/Universe.h>  // NOLINT

namespace ola {

/*
 * The interface for a Port
 */
class AbstractPort {
  public:
    AbstractPort() {}
    virtual ~AbstractPort() {}

    // return the device that this port belongs to
    virtual AbstractDevice *GetDevice() const = 0;
    // return the id of the port within this deivce
    virtual unsigned int PortId() const = 0;
    // return a globally unique id of this port. This is used to preserve port
    // universe bindings
    virtual string UniqueId() const = 0;
    // bind this port to a universe
    virtual bool SetUniverse(Universe *universe) = 0;
    // return the universe that this port is bound to or NULL
    virtual Universe *GetUniverse() const = 0;
    // signal the port that the DMX data has changed
    virtual int DmxChanged() = 0;

    // read/write dmx data to this port
    virtual bool WriteDMX(const DmxBuffer &buffer) = 0;
    virtual const DmxBuffer &ReadDMX() const = 0;

    // indicate our port's capability
    virtual bool IsOutput() const = 0;

    // return a short description of this port
    virtual string Description() const = 0;

    // Called if the universe name changes
    virtual void UniverseNameChanged(const string &new_name) = 0;
};


/*
 * A partial implementation of the Port interface templatized by the parent
 * Device.
 */
template <typename DeviceClass>
class Port: public AbstractPort {
  public:
    Port(DeviceClass *parent, unsigned int port_id):
      AbstractPort(),
      m_port_id(port_id),
      m_port_string(""),
      m_universe(NULL),
      m_parent(parent) {}
    virtual ~Port() {}

    DeviceClass *GetDevice() const { return m_parent; }
    unsigned int PortId() const { return m_port_id; }
    // An empty string means we don't preserve settings.
    virtual string UniqueId() const;
    bool SetUniverse(Universe *uni) {
      m_universe = uni;
      return true;
    }
    Universe *GetUniverse() const { return m_universe; }
    int DmxChanged() {
      if (m_universe)
        return m_universe->PortDataChanged(this);
      return 0;
    }

    // default is an output port
    virtual bool IsOutput()  const { return true; }

    virtual string Description() const { return ""; }

    virtual void UniverseNameChanged(const string &new_name) {
      (void) new_name;
    }

  private:
    Port(const Port&);
    Port& operator=(const Port&);

    unsigned int m_port_id;
    mutable string m_port_string;
    Universe *m_universe;  // universe this port belongs to
    DeviceClass *m_parent;  // pointer to the device this port belongs to
};


/*
 * Constructs a unique id for this port from the device id and the port id.
 */
template <typename DeviceClass>
string Port<DeviceClass>::UniqueId() const {
  if (m_port_string.empty()) {
    if (!GetDevice())
      return "";

    std::stringstream str;
    str << GetDevice()->UniqueId() << "-" << PortId();
    m_port_string = str.str();
  }
  return m_port_string;
}

}  // ola
#endif  // INCLUDE_OLAD_PORT_H_
