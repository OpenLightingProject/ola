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
 * OlaClient.h
 * This is the main client API to OLA.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLA_CLIENT_OLACLIENT_H_
#define INCLUDE_OLA_CLIENT_OLACLIENT_H_

#include <ola/DmxBuffer.h>
#include <ola/client/CallbackTypes.h>
#include <ola/client/ClientArgs.h>
#include <ola/client/ClientTypes.h>
#include <ola/base/Macro.h>
#include <ola/common.h>
#include <ola/io/Descriptor.h>
#include <ola/plugin_id.h>
#include <ola/rdm/UID.h>
#include <ola/rdm/UIDSet.h>
#include <ola/timecode/TimeCode.h>

#include <memory>
#include <string>

namespace ola {
namespace client {

/**
 * @class OlaClient ola/client/OlaClient.h
 * @brief The callback based C++ client for OLA.
 */
class OlaClient {
  public:
    explicit OlaClient(ola::io::ConnectedDescriptor *descriptor);
    ~OlaClient();

    /*
     * @brief Setup this client.
     * @returns true on success, false on failure
     */
    bool Setup();

    /*
     * @brief Close the ola connection.
     * @return true on sucess, false on failure
     */
    bool Stop();

    /**
     * @brief Set the close handler.
     */
    void SetCloseHandler(ola::SingleUseCallback0<void> *callback);

    /**
     * @brief Set the callback to be run when new DMX data arrives.
     * The DMX callback will be run when new data arrives for universes that
     * have been registered with RegisterUniverse().
     * @param callback the callback to run upon receiving new DMX data.
     */
    void SetDMXCallback(RepeatableDMXCallback *callback);

    /**
     * @brief Fetch the list of plugins loaded.
     * @param callback the PluginListCallback to be invoked upon completion.
     */
    void FetchPluginList(PluginListCallback *callback);

    /**
     * @brief Fetch the description for a plugin.
     * @param plugin_id the id of the plugin to fetch the state for.
     * @param callback the PluginDescriptionCallback to be invoked upon
     * completion.
     */
    void FetchPluginDescription(ola_plugin_id plugin_id,
                                PluginDescriptionCallback *callback);

    /**
     * @brief Fetch the state of a plugin. This returns the state and the list
     * of plugins this plugin conflicts with.
     * @param plugin_id the id of the plugin to fetch the state for.
     * @param callback the PluginStateCallback to be invoked upon completion.
     */
    void FetchPluginState(ola_plugin_id plugin_id,
                          PluginStateCallback *callback);

    /**
     * @brief Request a list of the available devices.
     * @param plugin_filter only fetch devices that belong to the specified
     * plugin.
     * @param callback the DeviceInfoCallback to be invoked upon completion.
     */
    void FetchDeviceInfo(ola_plugin_id plugin_filter,
                         DeviceInfoCallback *callback);

    /**
     * Request a list of ports that could be patched to new universe.
     * @param callback the CandidatePortsCallback invoked upon completion.
     */
    void FetchCandidatePorts(CandidatePortsCallback *callback);

    /**
     * @brief Request a list of ports that could be patched to a particular
     * universe.
     * @param universe_id the id of the universe to fetch the candidate ports
     * for.
     * @param callback the CandidatePortsCallback invoked upon completion.
     */
    void FetchCandidatePorts(unsigned int universe_id,
                             CandidatePortsCallback *callback);

    /**
     * @brief Send a device config request.
     * @param device_alias the device alias of the device to configure.
     * @param msg the blob of data to send to the device.
     * @param callback the ConfigureDeviceCallback invoked upon completion.
     */
    void ConfigureDevice(unsigned int device_alias,
                         const std::string &msg,
                         ConfigureDeviceCallback *callback);

    /**
     * @brief Set the priority for a port to inherit mode.
     * @param device_alias the device containing the port to change
     * @param port the port id of the port to change.
     * @param port_direction the direction of the port.
     * @param callback the SetCallback to invoke upon completion.
     */
    void SetPortPriorityInherit(unsigned int device_alias,
                                unsigned int port,
                                PortDirection port_direction,
                                SetCallback *callback);

    /**
     * @brief Set the priority for a port to override mode
     * @param device_alias the device containing the port to change
     * @param port the port id of the port to change.
     * @param port_direction the direction of the port.
     * @param value the new port priority value.
     * @param callback the SetCallback to invoke upon completion.
     */
    void SetPortPriorityOverride(unsigned int device_alias,
                                 unsigned int port,
                                 PortDirection port_direction,
                                 uint8_t value,
                                 SetCallback *callback);

    /**
     * @brief Request a list of universes.
     * @param callback the UniverseListCallback to invoke upon completion.
     */
    void FetchUniverseList(UniverseListCallback *callback);

    /**
     * @brief Fetch the information for a given universe.
     * @param universe the id of the universe.
     * @param callback the UniverseInfoCallback to invoke upon completion.
     */
    void FetchUniverseInfo(unsigned int universe,
                           UniverseInfoCallback *callback);

    /**
     * @brief Set the name of a universe.
     * @param universe the id of the universe
     * @param name the new name to use.
     * @param callback the SetCallback to invoke upon completion.
     */
    void SetUniverseName(unsigned int universe,
                         const std::string &name,
                         SetCallback *callback);

    /**
     * @brief Set the merge mode of a universe.
     * @param universe the id of the universe
     * @param mode the new merge mode
     * @param callback the SetCallback to invoke upon completion.
     */
    void SetUniverseMergeMode(unsigned int universe,
                              OlaUniverse::merge_mode mode,
                              SetCallback *callback);

    /**
     * @brief Patch or unpatch a port from a universe.
     * @param device_alias the device containing the port to change
     * @param port the port id of the port to change.
     * @param port_direction the direction of the port.
     * @param action OlaClientCore::PATCH or OlaClientCore::UNPATCH.
     * @param universe universe id to patch the port to.
     * @param callback the SetCallback to invoke upon completion.
     */
    void Patch(unsigned int device_alias,
               unsigned int port,
               PortDirection port_direction,
               PatchAction action,
               unsigned int universe,
               SetCallback *callback);

    /**
     * @brief Register our interest in a universe. The callback set by
     * SetDMXCallback() will be called when new DMX data arrives.
     * @param universe the id of the universe to register for.
     * @param register_action the action (register or unregister)
     * @param callback the SetCallback to invoke upon completion.
     */
    void RegisterUniverse(unsigned int universe,
                          RegisterAction register_action,
                          SetCallback *callback);

    /**
     * @brief Send DMX data.
     * @param universe the universe to send to.
     * @param data the DmxBuffer with the data
     * @param args the SendDMXArgs to use for this call.
     */
    void SendDMX(unsigned int universe,
                 const DmxBuffer &data,
                 const SendDMXArgs &args);

    /**
     * @brief Fetch the latest DMX data for a universe.
     * @param universe the universe id to get data for.
     * @param callback the SetCallback to invoke upon completion.
     */
    void FetchDMX(unsigned int universe, DMXCallback *callback);

    /**
     * @brief Trigger discovery for a universe.
     * @param universe the universe id to run discovery on.
     * @param discovery_type the type of discovery to run before returning.
     * @param callback the UIDListCallback to invoke upon completion.
     */
    void RunDiscovery(unsigned int universe,
                      DiscoveryType discovery_type,
                      DiscoveryCallback *callback);

    /**
     * @brief Set the source UID for this client.
     * @param uid the UID to use when sending RDM messages from this client.
     * @param callback the SetCallback to invoke upon completion.
     */
    void SetSourceUID(const ola::rdm::UID &uid, SetCallback *callback);

    /**
     * @brief Send an RDM Get Command.
     * @param universe the universe to send the command on
     * @param uid the UID to send the command to
     * @param sub_device the sub device index
     * @param pid the PID to address
     * @param data the optional data to send
     * @param data_length the length of the data
     * @param args the RDM arguments which includes the callback to run.
     */
    void RDMGet(unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length,
                const SendRDMArgs& args);

    /**
     * @brief Send an RDM Set Command.
     * @param universe the universe to send the command on
     * @param uid the UID to send the command to
     * @param sub_device the sub device index
     * @param pid the PID to address
     * @param data the optional data to send
     * @param data_length the length of the data
     * @param args the RDM arguments which includes the callback to run.
     */
    void RDMSet(unsigned int universe,
                const ola::rdm::UID &uid,
                uint16_t sub_device,
                uint16_t pid,
                const uint8_t *data,
                unsigned int data_length,
                const SendRDMArgs& args);

    /**
     * @brief Send TimeCode data.
     * @param timecode The timecode data.
     * @param callback the SetCallback to invoke when the send completes.
     */
    void SendTimeCode(const ola::timecode::TimeCode &timecode,
                      SetCallback *callback);

  private:
    std::auto_ptr<class OlaClientCore> m_core;

    DISALLOW_COPY_AND_ASSIGN(OlaClient);
};
}  // namespace client
}  // namespace ola
#endif  // INCLUDE_OLA_CLIENT_OLACLIENT_H_
