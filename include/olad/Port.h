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
 * Header file for the Port classes
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORT_H_
#define INCLUDE_OLAD_PORT_H_

#include <string>
#include <olad/PluginAdaptor.h>  // NOLINT
#include <ola/DmxBuffer.h>  // NOLINT
#include <ola/rdm/RDMCommand.h>  // NOLINT
#include <ola/rdm/RDMControllerInterface.h>  // NOLINT
#include <olad/DmxSource.h>  // NOLINT
#include <olad/PortConstants.h>  // NOLINT
#include <olad/Universe.h>  // NOLINT

namespace ola {

class AbstractDevice;

/*
 * The base port class, all ports inherit from this.
 */
class Port {
  public:
    virtual ~Port() {}

    // return the id of the port within this deivce
    virtual unsigned int PortId() const = 0;

    // Return the device which owns this port
    virtual AbstractDevice *GetDevice() const = 0;

    // return a short description of this port
    virtual string Description() const = 0;

    // bind this port to a universe
    virtual bool SetUniverse(Universe *universe) = 0;

    // return the universe that this port is bound to or NULL
    virtual Universe *GetUniverse() const = 0;

    // Return a globally unique id of this port. This is used to preserve port
    // universe bindings. An empty string means we don't preserve settings.
    virtual string UniqueId() const = 0;

    // this tells us what sort of priority capabilities this port has
    virtual port_priority_capability PriorityCapability() const = 0;

    virtual bool SetPriority(uint8_t priority) = 0;
    virtual uint8_t GetPriority() const = 0;

    virtual void SetPriorityMode(port_priority_mode mode) = 0;
    virtual port_priority_mode GetPriorityMode() const = 0;
};


/*
 * The Input Port interface, for ports that provide push data into the OLA
 * system.
 */
class InputPort: public Port {
  public:
    virtual ~InputPort() {}

    // signal the port that the DMX data has changed
    virtual void DmxChanged() = 0;

    // Get the current data
    virtual const DmxSource &SourceData() const = 0;

    // Handle RDMRequests, ownership of the request object is transferred
    virtual void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                                  ola::rdm::RDMCallback *callback) = 0;
};


/*
 * The Output Port interface, for ports that send data from the OLA system.
 */
class OutputPort: public Port {
  public:
    virtual ~OutputPort() {}

    // Write dmx data to this port
    virtual bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) = 0;

    // Called if the universe name changes
    virtual void UniverseNameChanged(const string &new_name) = 0;

    // Handle RDMRequests, ownership of the request object is transferred
    virtual void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                                  ola::rdm::RDMCallback *callback) = 0;
    virtual void RunRDMDiscovery() = 0;
    virtual void NewUIDList(const ola::rdm::UIDSet &uids) = 0;
};


/*
 * A Implementation of InputPort, provides the basic functionality which saves
 * the plugin implementations from having to do it.
 */
class BasicInputPort: public InputPort {
  public:
    BasicInputPort(AbstractDevice *parent,
                   unsigned int port_id,
                   const PluginAdaptor *plugin_adaptor);

    unsigned int PortId() const { return m_port_id; }
    AbstractDevice *GetDevice() const { return m_device; }
    bool SetUniverse(Universe *universe);
    Universe *GetUniverse() const { return m_universe; }
    virtual string UniqueId() const;
    bool SetPriority(uint8_t priority);
    uint8_t GetPriority() const { return m_priority; }
    void SetPriorityMode(port_priority_mode mode) { m_priority_mode = mode; }
    port_priority_mode GetPriorityMode() const { return m_priority_mode; }
    void DmxChanged();
    const DmxSource &SourceData() const { return m_dmx_source; }

    // rdm methods, the child class provides HandleRDMResponse
    void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                          ola::rdm::RDMCallback *callback);
    void TriggerRDMDiscovery();

    port_priority_capability PriorityCapability() const {
      return SupportsPriorities() ? CAPABILITY_FULL : CAPABILITY_STATIC;
    }

    // subclasses override these
    // Read the dmx data.
    virtual const DmxBuffer &ReadDMX() const = 0;

    // Get the inherited priority
    virtual uint8_t InheritedPriority() const {
      return DmxSource::PRIORITY_MIN;
    }

    // override this to cancel the SetUniverse operation.
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

  protected:
    // indicates whether this port supports priorities, default to no
    virtual bool SupportsPriorities() const { return false; }

  private:
    const unsigned int m_port_id;
    uint8_t m_priority;
    port_priority_mode m_priority_mode;
    mutable string m_port_string;
    Universe *m_universe;  // the universe this port belongs to
    AbstractDevice *m_device;
    DmxSource m_dmx_source;
    const PluginAdaptor *m_plugin_adaptor;

    BasicInputPort(const BasicInputPort&);
    BasicInputPort& operator=(const BasicInputPort&);
};


/*
 * An implementation of an OutputPort.
 */
class BasicOutputPort: public OutputPort {
  public:
    BasicOutputPort(AbstractDevice *parent,
                    unsigned int port_id,
                    bool start_rdm_discovery_on_patch = false);

    unsigned int PortId() const { return m_port_id; }
    AbstractDevice *GetDevice() const { return m_device; }
    bool SetUniverse(Universe *universe);
    Universe *GetUniverse() const { return m_universe; }
    string UniqueId() const;
    bool SetPriority(uint8_t priority);
    uint8_t GetPriority() const { return m_priority; }
    void SetPriorityMode(port_priority_mode mode) { m_priority_mode = mode; }
    port_priority_mode GetPriorityMode() const { return m_priority_mode; }

    // rdm methods, the child class provides HandleRDMRequest and
    // RunRDMDiscovery
    virtual void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                                  ola::rdm::RDMCallback *callback);
    virtual void RunRDMDiscovery();
    virtual void NewUIDList(const ola::rdm::UIDSet &uids);

    virtual void UniverseNameChanged(const string &new_name) {
      (void) new_name;
    }

    port_priority_capability PriorityCapability() const {
      return SupportsPriorities() ? CAPABILITY_FULL : CAPABILITY_NONE;
    }

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

  protected:
    // indicates whether this port supports priorities, default to no
    virtual bool SupportsPriorities() const { return false; }

  private:
    const unsigned int m_port_id;
    const bool m_discover_on_patch;
    uint8_t m_priority;
    port_priority_mode m_priority_mode;
    mutable string m_port_string;
    Universe *m_universe;  // the universe this port belongs to
    AbstractDevice *m_device;

    BasicOutputPort(const BasicOutputPort&);
    BasicOutputPort& operator=(const BasicOutputPort&);
};


/*
 * A Decorator for an Output Port.
 */
class OutputPortDecorator: public OutputPort {
  public:
    explicit OutputPortDecorator(OutputPort *port):
        m_port(port) {
    }

    ~OutputPortDecorator() {
      delete m_port;
    }

    virtual unsigned int PortId() const {
      return m_port->PortId();
    }

    virtual AbstractDevice *GetDevice() const {
      return m_port->GetDevice();
    }

    virtual string Description() const {
      return m_port->Description();
    }

    virtual bool SetUniverse(Universe *universe) {
      return m_port->SetUniverse(universe);
    }

    virtual Universe *GetUniverse() const {
      return m_port->GetUniverse();
    }

    virtual string UniqueId() const {
      return m_port->UniqueId();
    }

    virtual port_priority_capability PriorityCapability() const {
      return m_port->PriorityCapability();
    }

    virtual bool SetPriority(uint8_t priority) {
      return m_port->SetPriority(priority);
    }

    virtual uint8_t GetPriority() const {
      return m_port->GetPriority();
    }

    virtual void SetPriorityMode(port_priority_mode mode) {
      return m_port->SetPriorityMode(mode);
    }

    virtual port_priority_mode GetPriorityMode() const {
      return m_port->GetPriorityMode();
    }

    virtual void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                                  ola::rdm::RDMCallback *callback) {
      return m_port->HandleRDMRequest(request, callback);
    }

    virtual void RunRDMDiscovery() {
      return m_port->RunRDMDiscovery();
    }

    virtual void NewUIDList(const ola::rdm::UIDSet &uids) {
      return m_port->NewUIDList(uids);
    }

    virtual bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_port->WriteDMX(buffer, priority);
    }

    virtual void UniverseNameChanged(const string &new_name) {
      return m_port->UniverseNameChanged(new_name);
    }

  protected:
    OutputPort *m_port;
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
