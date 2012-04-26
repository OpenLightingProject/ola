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
 * OlaCallbackClient.cpp
 * Implementation of OlaCallbackClient
 * Copyright (C) 2010 Simon Newton
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/OlaCallbackClient.h"
#include "ola/OlaClientCore.h"
#include "ola/OlaDevice.h"
#include "ola/rdm/RDMAPI.h"
#include "ola/rdm/RDMAPIImplInterface.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {

using std::string;
using std::vector;
using ola::rdm::RDMAPIImplInterface;
using ola::io::ConnectedDescriptor;

OlaCallbackClient::OlaCallbackClient(ConnectedDescriptor *descriptor) {
  m_core = new OlaClientCore(descriptor);
}


OlaCallbackClient::~OlaCallbackClient() {
  delete m_core;
}


/*
 * Setup this client
 * @returns true on success, false on failure
 */
bool OlaCallbackClient::Setup() {
  return m_core->Setup();
}


/*
 * Close the ola connection.
 * @return true on sucess, false on failure
 */
bool OlaCallbackClient::Stop() {
  return m_core->Stop();
}


/*
 * Fetch the list of available plugins.
 * @returns true if the request succeeded, false otherwise.
 */
bool OlaCallbackClient::FetchPluginList(
    SingleUseCallback2<void,
                       const vector<class OlaPlugin>&,
                       const string&> *callback) {
  return m_core->FetchPluginList(callback);
}


/*
 * Fetch the description for a plugin.
 * @param plugin_id the id of the plugin to fetch.
 * @returns true if the request succeeded, false otherwise.
 */
bool OlaCallbackClient::FetchPluginDescription(
    ola_plugin_id plugin_id,
    SingleUseCallback2<void, const string&, const string&> *callback) {
  return m_core->FetchPluginDescription(plugin_id, callback);
}


/*
 * Request a listing of what devices are attached.
 * @param filter only fetch devices that belong to this plugin
 * @return true on success, false on failure
 */
bool OlaCallbackClient::FetchDeviceInfo(
    ola_plugin_id filter,
    SingleUseCallback2<void,
                       const vector <class OlaDevice>&,
                       const string&> *callback) {
  return m_core->FetchDeviceInfo(filter, callback);
}


/*
 * Request a list of what ports could be patched to a universe.
 * @param unique_id only fetch devices that belong to this plugin
 * @return true on success, false on failure
 */
bool OlaCallbackClient::FetchCandidatePorts(
        unsigned int universe_id,
        SingleUseCallback2<void,
                           const vector <class OlaDevice>&,
                           const string&> *callback) {
  return m_core->FetchCandidatePorts(universe_id, callback);
}


/*
 * Request a list of what ports could be patched to a new universe.
 * @return true on success, false on failure
 */
bool OlaCallbackClient::FetchCandidatePorts(
        SingleUseCallback2<void,
                           const vector <class OlaDevice>&,
                           const string&> *callback) {
  return m_core->FetchCandidatePorts(callback);
}


/*
 * Sends a device config request
 * @param device_alias the device_alias
 * @param msg  the request message
 * @return true on success, false on failure
 */
bool OlaCallbackClient::ConfigureDevice(
    unsigned int device_alias,
    const string &msg,
    SingleUseCallback2<void, const string&, const string&> *callback) {
  return m_core->ConfigureDevice(device_alias, msg, callback);
}



/*
 * Set the priority for a port to inherit mode
 * @param dev the device id
 * @param port the port id
 * @param port_direction the direction of the port
 * @param is_output true for an output port, false of an input port
 */
bool OlaCallbackClient::SetPortPriorityInherit(
    unsigned int device_alias,
    unsigned int port,
    PortDirection port_direction,
    SingleUseCallback1<void, const string&> *callback) {
  return m_core->SetPortPriorityInherit(device_alias,
                                        port,
                                        port_direction,
                                        callback);
}


/*
 * Set the priority for a port to override mode
 * @param dev the device id
 * @param port the port id
 * @param port_direction the direction of the port
 * @param value the port priority value
 */
bool OlaCallbackClient::SetPortPriorityOverride(
    unsigned int device_alias,
    unsigned int port,
    PortDirection port_direction,
    uint8_t value,
    SingleUseCallback1<void, const string&> *callback) {
  return m_core->SetPortPriorityOverride(device_alias,
                                         port,
                                         port_direction,
                                         value,
                                         callback);
}



/*
 * Request information about active universes.
 * @return true on success, false on failure
 */
bool OlaCallbackClient::FetchUniverseList(
    SingleUseCallback2<void,
                       const vector <class OlaUniverse>&,
                       const string &> *callback) {
  return m_core->FetchUniverseList(callback);
}


/*
 * Request information about a universe.
 * @return true on success, false on failure
 */
bool OlaCallbackClient::FetchUniverseInfo(
    unsigned int universe_id,
    SingleUseCallback2<void,
                       class OlaUniverse&,
                       const string &> *callback) {
  return m_core->FetchUniverseInfo(universe_id, callback);
}


/*
 * Set the name of a universe.
 * @param universe the id of the universe to set.
 * @param name the new universe name.
 * @return true on success, false on failure
 */
bool OlaCallbackClient::SetUniverseName(
    unsigned int universe,
    const string &name,
    SingleUseCallback1<void, const string&> *callback) {
  return m_core->SetUniverseName(universe, name, callback);
}


/*
 * Set the merge mode of a universe
 * @param universe the id of the universe to set.
 * @param mode the new universe merge mode.
 * @return true on success, false on failure
 */
bool OlaCallbackClient::SetUniverseMergeMode(
    unsigned int universe,
    OlaUniverse::merge_mode mode,
    SingleUseCallback1<void, const string&> *callback) {
  return m_core->SetUniverseMergeMode(universe, mode, callback);
}


/*
 * (Un)Patch a port to a universe
 * @param dev     the device id
 * @param port    the port id
 * @param port_direction the direction of the port
 * @param action  OlaClient::PATCH or OlaClient::UNPATCH
 * @param uni    universe id
 * @return true on success, false on failure
 */
bool OlaCallbackClient::Patch(
    unsigned int device_alias,
    unsigned int port,
    ola::PortDirection port_direction,
    ola::PatchAction action,
    unsigned int universe,
    SingleUseCallback1<void, const string&> *callback) {
  return m_core->Patch(device_alias,
                       port,
                       port_direction,
                       action,
                       universe,
                       callback);
}


/**
 * Set the callback to be run whenever new DMX data is received.
 */
void OlaCallbackClient::SetDmxCallback(
    Callback3<void,
              unsigned int,
              const DmxBuffer&, const string&> *callback) {
  m_core->SetDmxCallback(callback);
}



/*
 * Register our interest in a universe.
 * @param uni  the universe id
 * @param action REGISTER or UNREGISTER
 * @return true on success, false on failure
 */
bool OlaCallbackClient::RegisterUniverse(
    unsigned int universe,
    ola::RegisterAction register_action,
    SingleUseCallback1<void, const string&> *callback) {
  return m_core->RegisterUniverse(universe, register_action, callback);
}


/*
 * Write some dmx data.
 * @param universe universe to send to
 * @param data a DmxBuffer with the data
 * @return true on success, false on failure
 */
bool OlaCallbackClient::SendDmx(
    unsigned int universe,
    const DmxBuffer &data,
    SingleUseCallback1<void, const string&> *callback) {
  return m_core->SendDmx(universe, data, callback);
}


/*
 * Write some dmx data.
 * @param universe universe to send to
 * @param data a DmxBuffer with the data
 * @return true on success, false on failure
 */
bool OlaCallbackClient::SendDmx(
    unsigned int universe,
    const DmxBuffer &data,
    Callback1<void, const string&> *callback) {
  return m_core->SendDmx(universe, data, callback);
}


/*
 * Write some dmx data using the streaming dmx interface.
 * @param universe universe to send to
 * @param data a DmxBuffer with the data
 * @return true on success, false on failure
 */
bool OlaCallbackClient::SendDmx(
    unsigned int universe,
    const DmxBuffer &data) {
  return m_core->SendDmx(universe, data);
}


/*
 * Read dmx data.
 * @param universe the universe id to get data for
 * @return true on success, false on failure
 */
bool OlaCallbackClient::FetchDmx(
    unsigned int universe,
    SingleUseCallback2<void, const DmxBuffer&, const string&> *callback) {
  return m_core->FetchDmx(universe, callback);
}


/*
 * Fetch the UID list for a universe
 * @param universe the universe id to get data for
 * @return true on success, false on failure
 */
bool OlaCallbackClient::FetchUIDList(
    unsigned int universe,
    SingleUseCallback2<void,
                       const ola::rdm::UIDSet&,
                       const string&> *callback) {
  return m_core->FetchUIDList(universe, callback);
}


/*
 * run RDM discovery for a universe
 * @param universe the universe id to run discovery on
 * @param full: true for full discovery, false for incremental
 * @return true on success, false on failure
 */
bool OlaCallbackClient::RunDiscovery(
    unsigned int universe,
    bool full,
    ola::SingleUseCallback2<void,
                            const ola::rdm::UIDSet&,
                            const string&> *callback) {
  return m_core->RunDiscovery(universe, full, callback);
}


/*
 * Set this clients Source UID
 * @param uid the new source UID.
 * @return true on success, false on failure
 */
bool OlaCallbackClient::SetSourceUID(
    const ola::rdm::UID &uid,
    ola::SingleUseCallback1<void, const string&> *callback) {
  return m_core->SetSourceUID(uid, callback);
}



/*
 * Send an RDM Get Command
 * @param callback the Callback to invoke when this completes
 * @param universe the universe to send the command on
 * @param uid the UID to send the command to
 * @param sub_device the sub device index
 * @param pid the PID to address
 * @param data the optional data to send
 * @param data_length the length of the data
 * @return true on success, false on failure
 */
bool OlaCallbackClient::RDMGet(RDMAPIImplInterface::rdm_callback *callback,
                               unsigned int universe,
                               const ola::rdm::UID &uid,
                               uint16_t sub_device,
                               uint16_t pid,
                               const uint8_t *data,
                               unsigned int data_length) {
  return m_core->RDMGet(callback,
                        universe,
                        uid,
                        sub_device,
                        pid,
                        data,
                        data_length);
}


/*
 * Send an RDM Get Command and get the pid it returns.
 * @param callback the Callback to invoke when this completes
 * @param universe the universe to send the command on
 * @param uid the UID to send the command to
 * @param sub_device the sub device index
 * @param pid the PID to address
 * @param data the optional data to send
 * @param data_length the length of the data
 * @return true on success, false on failure
 */
bool OlaCallbackClient::RDMGet(
    RDMAPIImplInterface::rdm_pid_callback *callback,
    unsigned int universe,
    const ola::rdm::UID &uid,
    uint16_t sub_device,
    uint16_t pid,
    const uint8_t *data,
    unsigned int data_length) {
  return m_core->RDMGet(callback,
                        universe,
                        uid,
                        sub_device,
                        pid,
                        data,
                        data_length);
}


/*
 * Send an RDM Set Command
 * @param callback the Callback to invoke when this completes
 * @param universe the universe to send the command on
 * @param uid the UID to send the command to
 * @param sub_device the sub device index
 * @param pid the PID to address
 * @param data the optional data to send
 * @param data_length the length of the data
 * @return true on success, false on failure
 */
bool OlaCallbackClient::RDMSet(RDMAPIImplInterface::rdm_callback *callback,
                               unsigned int universe,
                               const ola::rdm::UID &uid,
                               uint16_t sub_device,
                               uint16_t pid,
                               const uint8_t *data,
                               unsigned int data_length) {
  return m_core->RDMSet(callback,
                        universe,
                        uid,
                        sub_device,
                        pid,
                        data,
                        data_length);
}


/**
 * Send TimeCode data.
 * @param callback the Callback to invoke when this completes
 * @param timecode The timecode data.
 * @return true on success, false on failure
 */
bool OlaCallbackClient::SendTimeCode(
    ola::SingleUseCallback1<void, const string&> *callback,
    const ola::timecode::TimeCode &timecode) {
  return m_core->SendTimeCode(callback, timecode);
}


/*
 * Send an RDM Set Command and get the pid it returns
 * @param callback the Callback to invoke when this completes
 * @param universe the universe to send the command on
 * @param uid the UID to send the command to
 * @param sub_device the sub device index
 * @param pid the PID to address
 * @param data the optional data to send
 * @param data_length the length of the data
 * @return true on success, false on failure
 */
bool OlaCallbackClient::RDMSet(RDMAPIImplInterface::rdm_pid_callback *callback,
                               unsigned int universe,
                               const ola::rdm::UID &uid,
                               uint16_t sub_device,
                               uint16_t pid,
                               const uint8_t *data,
                               unsigned int data_length) {
  return m_core->RDMSet(callback,
                        universe,
                        uid,
                        sub_device,
                        pid,
                        data,
                        data_length);
}
}  // ola
