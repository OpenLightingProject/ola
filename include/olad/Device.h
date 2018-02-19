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
 * Device.h
 * Header file for the Device class
 * Copyright (C) 2005 Simon Newton
 */

#ifndef INCLUDE_OLAD_DEVICE_H_
#define INCLUDE_OLAD_DEVICE_H_

#include <ola/base/Macro.h>
#include <olad/Port.h>
#include <stdint.h>
#include <map>
#include <string>
#include <vector>

namespace ola {
namespace rpc {
class RpcController;
}
}

namespace ola {

class AbstractPlugin;

/**
 * @brief The interface for a Device
 */
class AbstractDevice {
 public:
  typedef BaseCallback0<void> ConfigureCallback;

  AbstractDevice() {}
  virtual ~AbstractDevice() {}

  /** @brief The name of this Device */
  virtual const std::string Name() const = 0;

  /** @brief The Plugin that owns this Device */
  virtual AbstractPlugin *Owner() const = 0;

  /**
   * @brief The unique id of this Device
   * @note This is guaranteed to be unique and persist across restarts.
   */
  virtual std::string UniqueId() const = 0;

  /** @brief Stop the Device */
  virtual bool Stop() = 0;

  /** @brief Allow input and output ports to be patched to the same universe */
  virtual bool AllowLooping() const = 0;

  /**
   * @brief Allow multiple ports of the same type to be patched to the same
   * universe.
   */
  virtual bool AllowMultiPortPatching() const = 0;

  /** @brief Fetch a list of all ports in this Device */
  virtual void InputPorts(std::vector<InputPort*> *ports) const = 0;
  virtual void OutputPorts(std::vector<OutputPort*> *ports) const = 0;

  /** @brief Lookup a particular InputPort in this Device */
  virtual InputPort *GetInputPort(unsigned int port_id) const = 0;
  /** @brief Lookup a particular OutputPort in this Device */
  virtual OutputPort *GetOutputPort(unsigned int port_id) const = 0;

  /** @brief Configure this Device */
  virtual void Configure(ola::rpc::RpcController *controller,
                         const std::string &request,
                         std::string *response,
                         ConfigureCallback *done) = 0;
};


/**
 * A partial implementation of a Device.
 */
class Device: public AbstractDevice {
 public:
  Device(AbstractPlugin *owner, const std::string &name);
  virtual ~Device();

  const std::string Name() const { return m_name; }

  /** @brief Sets the name of this Device */
  void SetName(const std::string &name) { m_name = name; }

  AbstractPlugin *Owner() const { return m_owner; }
  std::string UniqueId() const;

  /**
   * @brief The device ID
   * @returns an id which is unique within the plugin,
   */
  virtual std::string DeviceId() const = 0;

  bool IsEnabled() const { return m_enabled; }

  /** @brief Start the Device */
  bool Start();
  bool Stop();

  // sane defaults
  bool AllowLooping() const { return false; }
  bool AllowMultiPortPatching() const { return false; }

  bool AddPort(InputPort *port);
  bool AddPort(OutputPort *port);
  void InputPorts(std::vector<InputPort*> *ports) const;
  void OutputPorts(std::vector<OutputPort*> *ports) const;

  InputPort *GetInputPort(unsigned int port_id) const;
  OutputPort *GetOutputPort(unsigned int port_id) const;

  /** @brief Free all ports */
  void DeleteAllPorts();

  // Handle a Configure request
  virtual void Configure(ola::rpc::RpcController *controller,
                         const std::string &request,
                         std::string *response,
                         ConfigureCallback *done);

 protected:
  /**
   * @brief Called during Start().
   *
   * This allows the subclass to perform device initialization.
   */
  virtual bool StartHook() { return true; }
  virtual void PrePortStop() {}
  virtual void PostPortStop() {}

 private:
  typedef std::map<unsigned int, InputPort*> input_port_map;
  typedef std::map<unsigned int, OutputPort*> output_port_map;

  bool m_enabled;
  AbstractPlugin *m_owner;  // which plugin owns this device
  std::string m_name;  // device name
  mutable std::string m_unique_id;  // device id
  input_port_map m_input_ports;
  output_port_map m_output_ports;

  template<class PortClass>
  bool GenericAddPort(PortClass *port,
                      std::map<unsigned int, PortClass*> *ports);

  template <class PortClass>
  void GenericDeletePort(PortClass *p);

  DISALLOW_COPY_AND_ASSIGN(Device);
};
}  // namespace ola
#endif  // INCLUDE_OLAD_DEVICE_H_
