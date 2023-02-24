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
 * ClientTypesFactory.h
 * This provides convenience functions to create data objects from
 * corresponding protocol buffer objects.
 * Copyright (C) 2015 Simon Marchi
 */

#ifndef OLA_CLIENTTYPESFACTORY_H_
#define OLA_CLIENTTYPESFACTORY_H_

#include <ola/client/ClientTypes.h>
#include <common/protocol/Ola.pb.h>

namespace ola {
namespace client {

/**
 * @brief Creates OlaPlugin types from protocol buffer objects.
 */
class ClientTypesFactory {
 public:
  static OlaPlugin PluginFromProtobuf(
      const ola::proto::PluginInfo &plugin_info);
  static OlaInputPort InputPortFromProtobuf(
      const ola::proto::PortInfo &port_info);
  static OlaOutputPort OutputPortFromProtobufOut(
      const ola::proto::PortInfo &port_info);
  static OlaDevice DeviceFromProtobuf(
      const ola::proto::DeviceInfo &device_info);
  static OlaUniverse UniverseFromProtobuf(
      const ola::proto::UniverseInfo &universe_info);
};

}  // namespace client
}  // namespace ola
#endif  // OLA_CLIENTTYPESFACTORY_H_
