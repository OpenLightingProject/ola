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
 * ClientArgs.h
 * Types used as arguments to the OLA Client.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_CLIENT_CLIENTARGS_H_
#define INCLUDE_OLA_CLIENT_CLIENTARGS_H_

#include <ola/client/CallbackTypes.h>
#include <ola/dmx/SourcePriorities.h>

/**
 * @file
 * @brief Types used as arguments for the OLA Client.
 */

namespace ola {
namespace client {

/**
 * @brief The patch action, used with OlaClient::Patch()
 */
enum PatchAction {
  PATCH,  /**< Patch the port. */
  UNPATCH,  /**< Unpatch the port */
};

/**
 * @brief The register action, used with OlaClient::RegisterUniverse()
 */
enum RegisterAction {
  REGISTER,  /**< Register for the universe */
  UNREGISTER,  /**< Unregister from the universe */
};

/**
 * @brief The port direction.
 */
enum PortDirection {
  INPUT_PORT,  /**< An input port which receives DMX data */
  OUTPUT_PORT,  /**< An output port which sends DMX data */
};

/**
 * @brief The type of discovery to run with OlaClient::RunDiscovery().
 */
enum DiscoveryType {
  DISCOVERY_CACHED,  /**< Fetch the cached list of UIDs */
  DISCOVERY_INCREMENTAL,  /**< Trigger incremental discovery */
  DISCOVERY_FULL,  /**< Trigger full discovery */
};

/**
 * @brief Arguments passed to the SendDmx() method.
 */
struct SendDmxArgs {
  /**
   * @brief The priority of the data, defaults to
   * ola::dmx::PRIORITY_DEFAULT.
   */
  uint8_t priority;
  /**
   * @brief the Callback to run upon completion. Defaults to NULL.
   */
  GeneralSetCallback *callback;

  /**
   * @brief Create a new SendDmxArgs object
   */
  SendDmxArgs()
      : priority(ola::dmx::SOURCE_PRIORITY_DEFAULT),
        callback(NULL) {
  }

  /**
   * @brief Create a new SendDmxArgs object
   */
  explicit SendDmxArgs(GeneralSetCallback *callback)
      : priority(ola::dmx::SOURCE_PRIORITY_DEFAULT),
        callback(callback) {
  }
};
}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_CLIENTARGS_H_
