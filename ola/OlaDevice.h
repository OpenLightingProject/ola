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
 * OlaDevice.h
 * Interface to the OLA Client Device class
 * Copyright (C) 2005-2006 Simon Newton
 */

#ifndef OLA_OLADEVICE_H_
#define OLA_OLADEVICE_H_

#include <olad/PortConstants.h>
#include <string>
#include <vector>

namespace ola {

using std::string;
using std::vector;


enum PatchAction {PATCH, UNPATCH};
enum RegisterAction {REGISTER, UNREGISTER};
enum PortDirection {INPUT_PORT, OUTPUT_PORT};


/*
 * Represents a Plugin
 */
class OlaPlugin {
  public:
    OlaPlugin(unsigned int id, const string &name):
      m_id(id),
      m_name(name) {}
    ~OlaPlugin() {}

    unsigned int Id() const { return m_id; }
    string Name() const { return m_name; }

    bool operator<(const OlaPlugin &other) const {
      return m_id < other.m_id;
    }
  private:
    unsigned int m_id;  // id of this plugin
    string m_name;  // plugin name
};


/*
 * Represents a port
 */
class OlaPort {
  public:
    OlaPort(unsigned int port_id,
            unsigned int universe,
            bool active,
            const string &description,
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

    unsigned int Universe() const { return m_universe; }
    bool IsActive() const { return m_active; }
    string Description() const { return m_description; }

    port_priority_capability PriorityCapability() const {
      return m_priority_capability;
    }
    port_priority_mode PriorityMode() const {
      return m_priority_mode;
    }
    uint8_t Priority() const { return m_priority; }

    bool SupportsRDM() const { return m_supports_rdm; }

  private:
    unsigned int m_id;  // id of this port
    unsigned int m_universe;  // universe
    bool m_active;  // active
    string m_description;
    port_priority_capability m_priority_capability;
    port_priority_mode m_priority_mode;
    uint8_t m_priority;
    bool m_supports_rdm;
};


class OlaInputPort: public OlaPort {
  public:
    OlaInputPort(unsigned int port_id,
                 unsigned int universe,
                 bool active,
                 const string &description,
                 port_priority_capability capability,
                 port_priority_mode mode,
                 uint8_t priority,
                 bool supports_rdm):
        OlaPort(port_id, universe, active, description,
                capability, mode, priority, supports_rdm) {
    }
};


class OlaOutputPort: public OlaPort {
  public:
    OlaOutputPort(unsigned int port_id,
                  unsigned int universe,
                  bool active,
                  const string &description,
                  port_priority_capability capability,
                  port_priority_mode mode,
                  uint8_t priority,
                  bool supports_rdm):
        OlaPort(port_id, universe, active, description,
                capability, mode, priority, supports_rdm) {
    }
};


/*
 * Represents a device
 */
class OlaDevice {
  public:
    OlaDevice(const string &id,
              unsigned int alias,
              const string &name,
              int plugin_id,
              const vector<OlaInputPort> &input_ports,
              const vector<OlaOutputPort> &output_ports):
      m_id(id),
      m_alias(alias),
      m_name(name),
      m_plugin_id(plugin_id),
      m_input_ports(input_ports),
      m_output_ports(output_ports) {}
    ~OlaDevice() {}

    string Id() const { return m_id; }
    unsigned int Alias() const { return m_alias; }
    string Name() const { return m_name; }
    int PluginId() const { return m_plugin_id; }

    const vector<OlaInputPort> &InputPorts() const { return m_input_ports; }
    const vector<OlaOutputPort> &OutputPorts() const { return m_output_ports; }

    bool operator<(const OlaDevice &other) const {
      return m_alias < other.m_alias;
    }

  private:
    string m_id;            // device id
    unsigned int m_alias;   // device alias
    std::string m_name;  // device name
    int m_plugin_id;     // parent plugin id
    std::vector<OlaInputPort> m_input_ports;
    std::vector<OlaOutputPort> m_output_ports;
};


/*
 * Represents a universe
 */
class OlaUniverse {
  public:
    enum merge_mode {
      MERGE_HTP,
      MERGE_LTP,
    };

    OlaUniverse(unsigned int id,
                merge_mode m,
                const string &name,
                unsigned int input_port_count,
                unsigned int output_port_count,
                unsigned int rdm_device_count):
      m_id(id),
      m_merge_mode(m),
      m_name(name),
      m_input_port_count(input_port_count),
      m_output_port_count(output_port_count),
      m_rdm_device_count(rdm_device_count) {}
    ~OlaUniverse() {}

    unsigned int Id() const { return m_id;}
    merge_mode MergeMode() const { return m_merge_mode; }
    string Name() const { return m_name;}
    unsigned int InputPortCount() const { return m_input_port_count; }
    unsigned int OutputPortCount() const { return m_output_port_count; }
    unsigned int RDMDeviceCount() const { return m_rdm_device_count; }

  private:
    unsigned int m_id;      // id of this universe
    merge_mode m_merge_mode;  // merge mode
    string m_name;    // universe name
    unsigned int m_input_port_count;
    unsigned int m_output_port_count;
    unsigned int m_rdm_device_count;
};
}  // ola
#endif  // OLA_OLADEVICE_H_
