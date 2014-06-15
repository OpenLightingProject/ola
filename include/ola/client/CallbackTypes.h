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
 * CallbackTypes.h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @file
 * @brief The various callbacks used with the OLA Client.
 */

#ifndef INCLUDE_OLA_CLIENT_CALLBACKTYPES_H_
#define INCLUDE_OLA_CLIENT_CALLBACKTYPES_H_

#include <ola/Callback.h>
#include <ola/DmxBuffer.h>
#include <ola/client/ClientTypes.h>
#include <ola/client/Result.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/UIDSet.h>

#include <string>
#include <vector>

namespace ola {
namespace client {

/**
 * @brief Invoked when OlaClient::FetchPluginList() completes.
 * @param result the Result of the API call.
 * @param plugins a vector of OlaPlugin objects.
 */
typedef SingleUseCallback2<void, const Result&, const std::vector<OlaPlugin>&>
    PluginListCallback;

/**
 * @brief Invoked when OlaClient::FetchPluginDescription() completes.
 * @param result the Result of the API call.
 * @param description the plugin description.
 */
typedef SingleUseCallback2<void, const Result&, const std::string&>
    PluginDescriptionCallback;

/**
 * @brief Invoked when OlaClient::FetchPluginState() completes.
 * @param result the Result of the API call.
 * @param state the PluginState object.
 */
typedef SingleUseCallback2<void, const Result&, const PluginState&>
  PluginStateCallback;

/**
 * @brief Invoked when OlaClient::FetchDeviceInfo() completes.
 * @param result the Result of the API call.
 * @param devices a vector of OlaDevice objects.
 */
typedef SingleUseCallback2<void, const Result&, const std::vector<OlaDevice>&>
    DeviceInfoCallback;

/**
 * @brief Invoked when OlaClient::FetchDeviceInfo() completes.
 * @param result the Result of the API call.
 * @param devices a vector of OlaDevice objects.
 */
typedef SingleUseCallback2<void, const Result&, const std::vector<OlaDevice>&>
    CandidatePortsCallback;

/**
 * @brief Invoked when an operation completes.
 * @param result the Result of the API call.
 */
typedef SingleUseCallback1<void, const Result&> SetCallback;

/**
 * @brief Invoked when an operation completes.
 * @param result the Result of the API call.
 */
typedef BaseCallback1<void, const Result&> GeneralSetCallback;

/**
 * @brief Invoked when OlaClient::FetchUniverseList() completes.
 * @param result the Result of the API call.
 * @param universes a vector of OlaUniverse objects
 */
typedef SingleUseCallback2<void, const Result&, const std::vector<OlaUniverse>&>
    UniverseListCallback;

/**
 * @brief Invoked when OlaClient::FetchUniverseInfo() completes.
 * @param result the Result of the API call.
 * @param universe the OlaUniverse object.
 */
typedef SingleUseCallback2<void, const Result&, const OlaUniverse&>
    UniverseInfoCallback;

/**
 * @brief Invoked when OlaClient::ConfigureDevice() completes.
 * @param result the Result of the API call.
 * @param response the raw data returned by the device.
 */
typedef SingleUseCallback2<void, const Result&, const std::string&>
    ConfigureDeviceCallback;

/**
 * @brief Invoked when OlaClient::RunDiscovery() completes.
 * @param result the Result of the API call.
 * @param uids the UIDSet containing the UIDs for the specified universe. The
 * UIDSet will be empty if the request failed.
 */
typedef SingleUseCallback2<void, const Result&, const ola::rdm::UIDSet&>
    DiscoveryCallback;


/**
 * @brief Called once when OlaClient::FetchDMX() completes.
 * @param result the Result of the API call.
 * @param metadata the DMXMetadata associated with the frame.
 * @param dmx the DmxBuffer with the data.
 */
typedef SingleUseCallback3<void, const Result&, const DMXMetadata&,
                           const DmxBuffer&> DMXCallback;

/**
 * @brief Called when new DMX data arrives.
 * @param metadata the DMXMetadata associated with the frame.
 * @param dmx the DmxBuffer with the data.
 */
typedef Callback2<void, const DMXMetadata&, const DmxBuffer&>
    RepeatableDMXCallback;

/**
 * @brief Called when a RDM request completes.
 * Used with OlaClient::RDMGet() and OlaClient::RDMSet().
 * @param result the Result of the API call.
 * @param metadata the metadata for the response, including the
 * rdm_response_code.
 * @param response the RDM Response, or NULL if no response was received.
 */
typedef SingleUseCallback3<void, const Result&,
                           const RDMMetadata&,
                           const ola::rdm::RDMResponse*> RDMCallback;


}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_CALLBACKTYPES_H_
