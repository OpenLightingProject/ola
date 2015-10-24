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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * ClientTypes.h
 * Types used as return values from the OLA Client.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_CLIENT_CLIENTTYPES_H_
#define INCLUDE_OLA_CLIENT_CLIENTTYPES_H_

#include <ola/dmx/SourcePriorities.h>
#include <ola/rdm/RDMFrame.h>
#include <ola/rdm/RDMResponseCodes.h>

#include <olad/PortConstants.h>

#include <string>
#include <vector>

/**
 * @file
 * @brief Types used as return values from the OLA Client.
 */

namespace ola {
namespace client {

/**
 * @brief Represents a Plugin.
 */
class OlaPlugin {
 public:
  OlaPlugin(unsigned int id,
            const std::string &name,
            bool active,
            bool enabled)
      : m_id(id),
        m_name(name),
        m_active(active),
        m_enabled(enabled) {}
  ~OlaPlugin() {}

  /**
   * @brief The plugin id.
   */
  unsigned int Id() const { return m_id; }

  /**
   * @brief The name of the plugin.
   */
  const std::string& Name() const { return m_name; }

  /**
   * @brief Indicates if the plugin is active or not
   * @return true if the plugin is active, false otherwise.
   */
  bool IsActive() const { return m_active; }

  /**
   * @brief Indicates if the plugin is enabled or not
   * @return true if the plugin is enabled, false otherwise.
   */
  bool IsEnabled() const { return m_enabled; }

  bool operator<(const OlaPlugin &other) const {
    return m_id < other.m_id;
  }

 private:
  unsigned int m_id;  // id of this plugin
  std::string m_name;  // plugin name
  bool m_active;
  bool m_enabled;
};

/**
 * @brief The state of a plugin.
 * This information can be used to detect conflicts between plugins.
 */
struct PluginState {
  /**
   * @brief The name of the plugin.
   */
  std::string name;
  /**
   * @brief true if the plugin is enabled.
   */
  bool enabled;
  /**
   * @brief true if the plugin is active.
   */
  bool active;
  /**
   * @brief The source of preferences for this plugin.
   */
  std::string preferences_source;
  /**
   * @brief A list on plugins which conflict with this one.
   */
  std::vector<OlaPlugin> conflicting_plugins;
};

/**
 * @brief The base class that represents a port.
 */
class OlaPort {
 public:
  OlaPort(unsigned int port_id,
          unsigned int universe,
          bool active,
          const std::string &description,
          port_priority_capability capability,
          port_priority_mode mode,
          uint8_t priority,
          bool supports_rdm):
    m_id(port_id),
    m_universe(universe),
    m_active(active),
    m_description(description),
    m_priority_capability(capability),
    m_priority_mode(mode),
    m_priority(priority),
    m_supports_rdm(supports_rdm) {}
  virtual ~OlaPort() {}

  unsigned int Id() const { return m_id; }

  /**
   * @brief The universe this port is patched to
   */
  unsigned int Universe() const { return m_universe; }

  bool IsActive() const { return m_active; }

  /**
   * @brief The description of this port
   */
  const std::string& Description() const { return m_description; }

  port_priority_capability PriorityCapability() const {
    return m_priority_capability;
  }
  port_priority_mode PriorityMode() const {
    return m_priority_mode;
  }

  uint8_t Priority() const { return m_priority; }

  /**
   * @brief Indicates if this port supports RDM
   * @returns true if the port supports RDM, false otherwise
   */
  bool SupportsRDM() const { return m_supports_rdm; }

 private:
  unsigned int m_id;  // id of this port
  unsigned int m_universe;  // universe
  bool m_active;  // active
  std::string m_description;
  port_priority_capability m_priority_capability;
  port_priority_mode m_priority_mode;
  uint8_t m_priority;
  bool m_supports_rdm;
};

/**
 * @brief An input port (receives DMX).
 */
class OlaInputPort: public OlaPort {
 public:
  OlaInputPort(unsigned int port_id,
               unsigned int universe,
               bool active,
               const std::string &description,
               port_priority_capability capability,
               port_priority_mode mode,
               uint8_t priority,
               bool supports_rdm):
      OlaPort(port_id, universe, active, description,
              capability, mode, priority, supports_rdm) {
  }
};

/**
 * @brief An Output Port (sends DMX).
 */
class OlaOutputPort: public OlaPort {
 public:
  OlaOutputPort(unsigned int port_id,
                unsigned int universe,
                bool active,
                const std::string &description,
                port_priority_capability capability,
                port_priority_mode mode,
                uint8_t priority,
                bool supports_rdm):
      OlaPort(port_id, universe, active, description,
              capability, mode, priority, supports_rdm) {
  }
};


/**
 * @brief Represents a device.
 */
class OlaDevice {
 public:
  OlaDevice(const std::string &id,
            unsigned int alias,
            const std::string &name,
            int plugin_id,
            const std::vector<OlaInputPort> &input_ports,
            const std::vector<OlaOutputPort> &output_ports):
    m_id(id),
    m_alias(alias),
    m_name(name),
    m_plugin_id(plugin_id),
    m_input_ports(input_ports),
    m_output_ports(output_ports) {}
  ~OlaDevice() {}

  std::string Id() const { return m_id; }
  unsigned int Alias() const { return m_alias; }
  const std::string& Name() const { return m_name; }
  int PluginId() const { return m_plugin_id; }

  const std::vector<OlaInputPort> &InputPorts() const {
    return m_input_ports;
  }
  const std::vector<OlaOutputPort> &OutputPorts() const {
    return m_output_ports;
  }

  bool operator<(const OlaDevice &other) const {
    return m_alias < other.m_alias;
  }

 private:
  std::string m_id;            // device id
  unsigned int m_alias;   // device alias
  std::string m_name;  // device name
  int m_plugin_id;     // parent plugin id
  std::vector<OlaInputPort> m_input_ports;
  std::vector<OlaOutputPort> m_output_ports;
};


/**
 * @brief Represents a universe.
 */
class OlaUniverse {
 public:
  enum merge_mode {
    MERGE_HTP,
    MERGE_LTP,
  };

  OlaUniverse(unsigned int id,
              merge_mode m,
              const std::string &name,
              const std::vector<OlaInputPort> &input_ports,
              const std::vector<OlaOutputPort> &output_ports,
              unsigned int rdm_device_count):
    m_id(id),
    m_merge_mode(m),
    m_name(name),
    m_input_ports(input_ports),
    m_output_ports(output_ports),
    m_rdm_device_count(rdm_device_count) {}
  ~OlaUniverse() {}

  unsigned int Id() const { return m_id;}
  merge_mode MergeMode() const { return m_merge_mode; }
  const std::string& Name() const { return m_name;}
  unsigned int InputPortCount() const { return m_input_ports.size(); }
  unsigned int OutputPortCount() const { return m_output_ports.size(); }
  unsigned int RDMDeviceCount() const { return m_rdm_device_count; }

  const std::vector<OlaInputPort> &InputPorts() const {
    return m_input_ports;
  }
  const std::vector<OlaOutputPort> &OutputPorts() const {
    return m_output_ports;
  }

 private:
  unsigned int m_id;
  merge_mode m_merge_mode;
  std::string m_name;
  std::vector<OlaInputPort> m_input_ports;
  std::vector<OlaOutputPort> m_output_ports;
  unsigned int m_rdm_device_count;
};

/**
 * @brief Metadata that accompanies DMX packets
 */
struct DMXMetadata {
  /**
   * @brief The universe the DMX frame is for.
   */
  unsigned int universe;
  /**
   * @brief The priority of the DMX frame.
   */
  uint8_t priority;

  explicit DMXMetadata(unsigned int _universe,
                       uint8_t _priority = ola::dmx::SOURCE_PRIORITY_DEFAULT)
      : universe(_universe),
        priority(_priority) {
  }
};


/**
 * @brief Metadata that accompanies RDM Responses.
 */
struct RDMMetadata {
  /**
   * @brief The internal (OLA) response code.
   */
  ola::rdm::rdm_response_code response_code;

  /**
   * @brief The RDMFrames that made up this response.
   */
  std::vector<ola::rdm::RDMFrame> frames;

  /**
   * @brief Construct a new RDMMetadata object.
   * The default response code is RDM_FAILED_TO_SEND.
   */
  RDMMetadata() : response_code(ola::rdm::RDM_FAILED_TO_SEND) {}

  explicit RDMMetadata(ola::rdm::rdm_response_code _response_code)
      : response_code(_response_code) {
  }
};
}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_CLIENTTYPES_H_
