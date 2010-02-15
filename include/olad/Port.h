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
#include <ola/Clock.h>
#include <olad/Universe.h>  // NOLINT
#include <olad/PortConstants.h>

namespace ola {

class AbstractDevice;

/*
 * The base port class.
 */
class Port {
  public:
    static const uint8_t PORT_PRIORITY_MIN;
    static const uint8_t PORT_PRIORITY_MAX;
    static const uint8_t PORT_PRIORITY_DEFAULT;

    Port(AbstractDevice *parent, unsigned int port_id)
        : m_port_id(port_id),
          m_priority(PORT_PRIORITY_DEFAULT),
          m_priority_mode(PRIORITY_MODE_INHERIT),
          m_port_string(""),
          m_universe(NULL),
          m_device(parent) {
    }
    virtual ~Port() {}

    // return the id of the port within this deivce
    virtual unsigned int PortId() const { return m_port_id; }

    // Return the device which owns this port
    AbstractDevice *GetDevice() const { return m_device; }

    // return a short description of this port
    virtual string Description() const = 0;

    // bind this port to a universe
    virtual bool SetUniverse(Universe *universe);

    // return the universe that this port is bound to or NULL
    virtual Universe *GetUniverse() const { return m_universe; }

    // Return a globally unique id of this port. This is used to preserve port
    // universe bindings. An empty string means we don't preserve settings.
    virtual string UniqueId() const;

    // Subclasses can override this to cancel the SetUniverse operation.
    virtual bool PreSetUniverse(Universe *old_universe,
                                Universe *new_universe) {
      (void) old_universe;
      (void) new_universe;
      return true;
    }

    virtual void PostSetUniverse(Universe *old_universe,
                                 Universe *new_universe) {
      (void) old_universe;
      (void) new_universe;
    }

    // this tells us what sort of priority capabilities this port has
    virtual port_priority_capability PriorityCapability() const = 0;

    bool SetPriority(uint8_t priority);
    uint8_t GetPriority() const { return m_priority; }

    void SetPriorityMode(port_priority_mode mode) { m_priority_mode = mode; }
    port_priority_mode GetPriorityMode() const { return m_priority_mode; }

  protected:
    virtual string PortPrefix() const = 0;

  private:
    const unsigned int m_port_id;
    uint8_t m_priority;
    port_priority_mode m_priority_mode;
    mutable string m_port_string;
    Universe *m_universe;  // universe this port belongs to
    AbstractDevice *m_device;

    Port(const Port&);
    Port& operator=(const Port&);
};


/*
 * A InputPort, these are Ports which receive data.
 */
class InputPort: public Port {
  public:
    InputPort(AbstractDevice *parent, unsigned int port_id)
        : Port(parent, port_id) {}

    // signal the port that the DMX data has changed
    virtual int DmxChanged() {
      if (GetUniverse()) {
        m_update_time.SetToCurrentTime();
        GetUniverse()->PortDataChanged(this);
      }
      return 0;
    }

    // read/write dmx data to this port
    virtual const DmxBuffer &ReadDMX() const = 0;

    port_priority_capability PriorityCapability() const {
      return SupportsPriorities() ? CAPABILITY_FULL : CAPABILITY_STATIC;
    }

    TimeStamp LastUpdateTime() const {
      return m_update_time;
    }

  protected:
    virtual string PortPrefix() const { return "I"; }

    // indicates whether this port supports priorities, default to no
    virtual bool SupportsPriorities() const { return false; }

  private:
    TimeStamp m_update_time;
};


/*
 * An OutputPort, these are ports that send data.
 */
class OutputPort: public Port {
  public:

    OutputPort(AbstractDevice *parent, unsigned int port_id)
        : Port(parent, port_id) {
    }

    // Write dmx data to this port
    virtual bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) = 0;

    // Called if the universe name changes
    virtual void UniverseNameChanged(const string &new_name) {
      (void) new_name;
    }

    port_priority_capability PriorityCapability() const {
      return SupportsPriorities() ? CAPABILITY_FULL : CAPABILITY_NONE;
    }

  protected:
    virtual string PortPrefix() const { return "O"; }

    // indicates whether this port supports priorities, default to no
    virtual bool SupportsPriorities() const { return false; }
};


/*
 * This allows switching based on Port type.
 */
template<class PortClass>
bool IsInputPort();

template<>
bool IsInputPort<OutputPort>();
}  // ola
#endif  // INCLUDE_OLAD_PORT_H_
