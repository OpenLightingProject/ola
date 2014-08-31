/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Port.h
 * Header file for the Port classes
 * Copyright (C) 2005 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORT_H_
#define INCLUDE_OLAD_PORT_H_

#include <ola/DmxBuffer.h>
#include <ola/base/Macro.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/timecode/TimeCode.h>
#include <olad/DmxSource.h>
#include <olad/PluginAdaptor.h>
#include <olad/PortConstants.h>
#include <olad/Universe.h>

#include <string>

namespace ola {

class AbstractDevice;

/**
 * The base port class, all ports inherit from this.
 */
class Port {
 public:
  virtual ~Port() {}

  /**
   * @brief Get the port ID
   * @return the id of the port within this device
   */
  virtual unsigned int PortId() const = 0;

  /**
   * @brief Get the device which owns this port
   * @return the device which owns this port
   */
  virtual AbstractDevice *GetDevice() const = 0;

  /**
   * @brief Get the port description
   * @return a short description of this port
   */
  virtual std::string Description() const = 0;

  /**
   * @brief Bind this port to a universe
   * @param universe the universe to bind to
   */
  virtual bool SetUniverse(Universe *universe) = 0;

  /**
   * @brief Get the universe this port is bound to
   * @return the universe that this port is bound to or NULL
   */
  virtual Universe *GetUniverse() const = 0;

  /**
   * @brief Return a globally unique id of this port.
   *
   * This is used to preserve port universe bindings. An empty string means
   * we don't preserve settings.
   */
  virtual std::string UniqueId() const = 0;

  /**
   * @brief This tells us what sort of priority capabilities this port has
   * @return a port_priority_capability
   */
  virtual port_priority_capability PriorityCapability() const = 0;

  virtual bool SetPriority(uint8_t priority) = 0;
  virtual uint8_t GetPriority() const = 0;

  virtual void SetPriorityMode(port_priority_mode mode) = 0;
  virtual port_priority_mode GetPriorityMode() const = 0;

  /**
   * @brief If this port supports RDM or not
   * @return true if RDM is supported, false otherwise
   */
  virtual bool SupportsRDM() const = 0;
};


/**
 * The Input Port interface, for ports that provide push data into the OLA
 * system.
 */
class InputPort: public Port {
 public:
  virtual ~InputPort() {}

  /**
   * @brief Signal to the port that the DMX data has changed
   */
  virtual void DmxChanged() = 0;

  /**
   * @brief Get the current DMX data
   */
  virtual const DmxSource &SourceData() const = 0;

  /**
   * @brief Handle RDMRequests, ownership of the RDMRequest object is transferred
   */
  virtual void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                                ola::rdm::RDMCallback *callback) = 0;
};


/**
 * The Output Port interface, for ports that send data from the OLA system.
 */
class OutputPort: public Port, ola::rdm::DiscoverableRDMControllerInterface {
 public:
  virtual ~OutputPort() {}

  /**
   * @brief Write DMX data to this port
   * @param buffer the DmxBuffer to write
   * @param priority the priority of the DMX data
   * @return true on success, false on failure
   */
  virtual bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) = 0;

  /**
   * @brief Called if the universe name changes
   */
  virtual void UniverseNameChanged(const std::string &new_name) = 0;

  // Methods from DiscoverableRDMControllerInterface
  // Ownership of the request object is transferred
  virtual void SendRDMRequest(const ola::rdm::RDMRequest *request,
                              ola::rdm::RDMCallback *callback) = 0;
  virtual void RunFullDiscovery(
      ola::rdm::RDMDiscoveryCallback *on_complete) = 0;
  virtual void RunIncrementalDiscovery(
      ola::rdm::RDMDiscoveryCallback *on_complete) = 0;

  // timecode support
  virtual bool SupportsTimeCode() const = 0;
  virtual bool SendTimeCode(const ola::timecode::TimeCode &timecode) = 0;
};


/**
 * A Implementation of InputPort, provides the basic functionality which saves
 * the plugin implementations from having to do it.
 */
class BasicInputPort: public InputPort {
 public:
  /**
   * @brief Create a new basic input port
   */
  BasicInputPort(AbstractDevice *parent,
                 unsigned int port_id,
                 const PluginAdaptor *plugin_adaptor,
                 bool supports_rdm = false);

  unsigned int PortId() const { return m_port_id; }
  AbstractDevice *GetDevice() const { return m_device; }
  bool SetUniverse(Universe *universe);
  Universe *GetUniverse() const { return m_universe; }
  virtual std::string UniqueId() const;
  bool SetPriority(uint8_t priority);
  uint8_t GetPriority() const { return m_priority; }
  void SetPriorityMode(port_priority_mode mode) { m_priority_mode = mode; }
  port_priority_mode GetPriorityMode() const { return m_priority_mode; }

  /**
   * @brief Called when there is new data for this port
   */
  void DmxChanged();
  const DmxSource &SourceData() const { return m_dmx_source; }

  // RDM methods, the child class provides HandleRDMResponse
  /**
   * @brief Handle an RDM Request on this port.
   * @param request the RDMRequest object, ownership is transferred to us
   * @param callback the callback to run
   */
  void HandleRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback);

  /**
   * @brief Trigger the RDM Discovery procedure for this universe
   */
  void TriggerRDMDiscovery(ola::rdm::RDMDiscoveryCallback *on_complete,
                           bool full = true);

  port_priority_capability PriorityCapability() const {
    return SupportsPriorities() ? CAPABILITY_FULL : CAPABILITY_STATIC;
  }

  // subclasses override these
  // Read the dmx data.
  virtual const DmxBuffer &ReadDMX() const = 0;

  // Get the inherited priority
  virtual uint8_t InheritedPriority() const {
    return ola::dmx::SOURCE_PRIORITY_MIN;
  }

  // override this to cancel the SetUniverse operation.
  virtual bool PreSetUniverse(Universe *, Universe *) { return true; }

  virtual void PostSetUniverse(Universe *, Universe *) {}

  virtual bool SupportsRDM() const { return m_supports_rdm; }

 protected:
  // indicates whether this port supports priorities, default to no
  virtual bool SupportsPriorities() const { return false; }

 private:
  const unsigned int m_port_id;
  uint8_t m_priority;
  port_priority_mode m_priority_mode;
  mutable std::string m_port_string;
  Universe *m_universe;  // the universe this port belongs to
  AbstractDevice *m_device;
  DmxSource m_dmx_source;
  const PluginAdaptor *m_plugin_adaptor;
  bool m_supports_rdm;

  DISALLOW_COPY_AND_ASSIGN(BasicInputPort);
};


/**
 * An implementation of an OutputPort.
 */
class BasicOutputPort: public OutputPort {
 public:
  /**
   * @brief Create a new BasicOutputPort
   */
  BasicOutputPort(AbstractDevice *parent,
                  unsigned int port_id,
                  bool start_rdm_discovery_on_patch = false,
                  bool supports_rdm = false);

  unsigned int PortId() const { return m_port_id; }
  AbstractDevice *GetDevice() const { return m_device; }
  bool SetUniverse(Universe *universe);
  Universe *GetUniverse() const { return m_universe; }
  std::string UniqueId() const;
  bool SetPriority(uint8_t priority);
  uint8_t GetPriority() const { return m_priority; }
  void SetPriorityMode(port_priority_mode mode) { m_priority_mode = mode; }
  port_priority_mode GetPriorityMode() const { return m_priority_mode; }

  virtual void UniverseNameChanged(const std::string &new_name) {
    (void) new_name;
  }

  port_priority_capability PriorityCapability() const {
    return SupportsPriorities() ? CAPABILITY_FULL : CAPABILITY_NONE;
  }

  // DiscoverableRDMControllerInterface methods
  /**
   * @brief Handle an RDMRequest, subclasses can implement this to support RDM
   */
  virtual void SendRDMRequest(const ola::rdm::RDMRequest *request,
                              ola::rdm::RDMCallback *callback);
  /**
   * @brief This is a noop for ports that don't support RDM
   */
  virtual void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *on_complete);

  /**
   * @brief This is a noop for ports that don't support RDM
   */
  virtual void RunIncrementalDiscovery(
      ola::rdm::RDMDiscoveryCallback *on_complete);

  // TimeCode
  virtual bool SupportsTimeCode() const { return false; }

  /**
   * @brief This is a noop for ports that don't support TimeCode
   */
  virtual bool SendTimeCode(const ola::timecode::TimeCode &) {
    return true;
  }

  // Subclasses can override this to cancel the SetUniverse operation.
  virtual bool PreSetUniverse(Universe *, Universe *) { return true; }
  virtual void PostSetUniverse(Universe *, Universe *) { }

  virtual bool SupportsRDM() const { return m_supports_rdm; }

 protected:
  // indicates whether this port supports priorities, default to no
  virtual bool SupportsPriorities() const { return false; }

  /**
   * @brief Called when the discovery triggered by patching completes
   */
  void UpdateUIDs(const ola::rdm::UIDSet &uids);

 private:
  const unsigned int m_port_id;
  const bool m_discover_on_patch;
  uint8_t m_priority;
  port_priority_mode m_priority_mode;
  mutable std::string m_port_string;
  Universe *m_universe;  // the universe this port belongs to
  AbstractDevice *m_device;
  bool m_supports_rdm;

  DISALLOW_COPY_AND_ASSIGN(BasicOutputPort);
};


/**
 * @brief This allows switching based on Port type.
 */
template<class PortClass>
bool IsInputPort();

template<>
bool IsInputPort<OutputPort>();
}  // namespace ola
#endif  // INCLUDE_OLAD_PORT_H_
