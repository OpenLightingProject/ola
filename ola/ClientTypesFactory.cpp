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
 * ClientTypesFactory.cpp
 * Copyright (C) 2015 Simon Marchi
 */

#include <vector>

#include "ola/ClientTypesFactory.h"

namespace ola {
namespace client {

using std::vector;

/*
 * Create an OlaPlugin object from a protobuf.
 */
OlaPlugin ClientTypesFactory::PluginFromProtobuf(
    const ola::proto::PluginInfo &plugin_info) {
  return OlaPlugin(plugin_info.plugin_id(),
                   plugin_info.name(),
                   plugin_info.active(),
                   plugin_info.enabled());
}

/*
 * Create an OlaInputPort object from a protobuf.
 */
OlaInputPort ClientTypesFactory::InputPortFromProtobuf(
    const ola::proto::PortInfo &port_info) {
  return OlaInputPort(port_info.port_id(),
                      port_info.universe(),
                      port_info.active(),
                      port_info.description(),
                      static_cast<port_priority_capability>(
                          port_info.priority_capability()),
                      static_cast<port_priority_mode>(
                          port_info.priority_mode()),
                      port_info.priority(),
                      port_info.supports_rdm());
}

/*
 * Create an OlaOutputPort object from a protobuf.
 */
OlaOutputPort ClientTypesFactory::OutputPortFromProtobufOut(
    const ola::proto::PortInfo &port_info) {
  return OlaOutputPort(port_info.port_id(),
                       port_info.universe(),
                       port_info.active(),
                       port_info.description(),
                       static_cast<port_priority_capability>(
                           port_info.priority_capability()),
                       static_cast<port_priority_mode>(
                           port_info.priority_mode()),
                       port_info.priority(),
                       port_info.supports_rdm());
}

/*
 * Create an OlaDevice object from a protobuf.
 */
OlaDevice ClientTypesFactory::DeviceFromProtobuf(
    const ola::proto::DeviceInfo &device_info) {
  vector<OlaInputPort> input_ports;
  for (int i = 0; i < device_info.input_port_size(); ++i) {
    ola::proto::PortInfo port_info = device_info.input_port(i);
    input_ports.push_back(
        ClientTypesFactory::InputPortFromProtobuf(port_info));
  }

  vector<OlaOutputPort> output_ports;
  for (int i = 0; i < device_info.output_port_size(); ++i) {
    ola::proto::PortInfo port_info = device_info.output_port(i);
    output_ports.push_back(
        ClientTypesFactory::OutputPortFromProtobufOut(port_info));
  }

  return OlaDevice(device_info.device_id(),
                   device_info.device_alias(),
                   device_info.device_name(),
                   device_info.plugin_id(),
                   input_ports,
                   output_ports);
}

/*
 * Create an OlaUniverse object from a protobuf.
 */
OlaUniverse ClientTypesFactory::UniverseFromProtobuf(
    const ola::proto::UniverseInfo &universe_info) {
  OlaUniverse::merge_mode merge_mode =
    universe_info.merge_mode() == ola::proto::HTP ?
    OlaUniverse::MERGE_HTP: OlaUniverse::MERGE_LTP;

  vector<OlaInputPort> input_ports;
  for (int j = 0; j < universe_info.input_ports_size(); ++j) {
    ola::proto::PortInfo port_info = universe_info.input_ports(j);
    input_ports.push_back(
        ClientTypesFactory::InputPortFromProtobuf(port_info));
  }

  vector<OlaOutputPort> output_ports;
  for (int j = 0; j < universe_info.output_ports_size(); ++j) {
    ola::proto::PortInfo port_info = universe_info.output_ports(j);
    output_ports.push_back(
        ClientTypesFactory::OutputPortFromProtobufOut(port_info));
  }

  return OlaUniverse(universe_info.universe(),
                     merge_mode,
                     universe_info.name(),
                     input_ports,
                     output_ports,
                     universe_info.rdm_devices());
}

}  // namespace client
}  // namespace ola
