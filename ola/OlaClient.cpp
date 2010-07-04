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
 * OlaClient.cpp
 * Implementation of OlaClient
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include "ola/OlaClient.h"
#include "ola/OlaClientCore.h"

namespace ola {

using ola::network::ConnectedSocket;

OlaClient::OlaClient(ConnectedSocket *socket) {
  m_core = new OlaClientCore(socket);
}

OlaClient::~OlaClient() {
  delete m_core;
}


/*
 * Setup this client
 *
 * @returns true on success, false on failure
 */
bool OlaClient::Setup() {
  return m_core->Setup();
}


/*
 * Close the ola connection.
 *
 * @return true on sucess, false on failure
 */
bool OlaClient::Stop() {
  return m_core->Stop();
}


/*
 * Set the OlaClientObserver object
 *
 * @params observer the OlaClientObserver object to be used for the callbacks.
 */
bool OlaClient::SetObserver(OlaClientObserver *observer) {
  return m_core->SetObserver(observer);
}


/*
 * Fetch info about available plugins. This results in a call to
 *   observer->Plugins(...)
 *  when the request returns.
 *
 * @params filter use this to filter on plugin id
 * @params include_description set to true to get the plugin descriptions as
 * well.
 * @returns true if the request succeeded, false otherwise.
 */
bool OlaClient::FetchPluginInfo(ola_plugin_id filter,
                                bool include_description) {
  return m_core->FetchPluginInfo(filter, include_description);
}


/*
 * Write some dmx data.
 *
 * @param universe   universe to send to
 * @param data  a DmxBuffer with the data
 * @return true on success, false on failure
 */
bool OlaClient::SendDmx(unsigned int universe, const DmxBuffer &data) {
  return m_core->SendDmx(universe, data);
}


/*
 * Read dmx data. This results in a call to
 *  observer->NewDmx()
 * when the request returns.
 *
 * @param universe the universe id to get data for
 * @return true on success, false on failure
 */
bool OlaClient::FetchDmx(unsigned int universe) {
  return m_core->FetchDmx(universe);
}


/*
 * Fetch the UID list for a universe
 * @param universe the universe id to get data for
 * @return true on success, false on failure
 */
bool OlaClient::FetchUIDList(unsigned int universe) {
  return m_core->FetchUIDList(universe);
}


/*
 * Force RDM discovery for a universe
 * @param universe the universe id to run discovery on
 * @return true on success, false on failure
 */
bool OlaClient::ForceDiscovery(unsigned int universe) {
  return m_core->ForceDiscovery(universe);
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

bool OlaClient::RDMGet(rdm_callback *callback,
            unsigned int universe,
            const UID &uid,
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
bool OlaClient::RDMSet(rdm_callback *callback,
                       unsigned int universe,
                       const UID &uid,
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
 * Request a listing of what devices are attached. This results in a call to
 *   observer->Devices()
 * when the request returns.
 *
 * @param filter only fetch devices that belong to this plugin
 * @return true on success, false on failure
 */
bool OlaClient::FetchDeviceInfo(ola_plugin_id filter) {
  return m_core->FetchDeviceInfo(filter);
}


/*
 * Request information about active universes. This results in a call to
 *   observer->Universes()
 * when the request returns.
 *
 * @return true on success, false on failure
 */
bool OlaClient::FetchUniverseInfo() {
  return m_core->FetchUniverseInfo();
}


/*
 * Set the name of a universe.
 *
 * @return true on success, false on failure
 */
bool OlaClient::SetUniverseName(unsigned int universe, const string &name) {
  return m_core->SetUniverseName(universe, name);
}


/*
 * Set the merge mode of a universe
 *
 * @return true on success, false on failure
 */
bool OlaClient::SetUniverseMergeMode(unsigned int universe,
                                     OlaUniverse::merge_mode mode) {
  return m_core->SetUniverseMergeMode(universe, mode);
}


/*
 * Register our interest in a universe. This results in calls to
 *   observer->NewDmx()
 * whenever the dmx values change.
 *
 * @param uni  the universe id
 * @param action REGISTER or UNREGISTER
 * @return true on success, false on failure
 */
bool OlaClient::RegisterUniverse(unsigned int universe,
                                 ola::RegisterAction register_action) {
  return m_core->RegisterUniverse(universe, register_action);
}


/*
 * (Un)Patch a port to a universe
 *
 * @param dev     the device id
 * @param port    the port id
 * @param is_output true for an output port, false of an input port
 * @param action  OlaClient::PATCH or OlaClient::UNPATCH
 * @param uni    universe id
 * @return true on success, false on failure
 */
bool OlaClient::Patch(unsigned int device_id,
                      unsigned int port_id,
                      bool is_output,
                      ola::PatchAction patch_action,
                      unsigned int universe) {
  return m_core->Patch(device_id, port_id, is_output, patch_action, universe);
}


/*
 * Set the priority for a port to inherit mode
 * @param dev the device id
 * @param port the port id
 * @param is_output true for an output port, false of an input port
 */
bool OlaClient::SetPortPriorityInherit(unsigned int device_alias,
                                       unsigned int port,
                                       bool is_output) {
  return m_core->SetPortPriorityInherit(device_alias, port, is_output);
}


/*
 * Set the priority for a port to override mode
 * @param dev the device id
 * @param port the port id
 * @param is_output true for an output port, false of an input port
 * @param value the port priority value
 */
bool OlaClient::SetPortPriorityOverride(unsigned int device_alias,
                                        unsigned int port,
                                        bool is_output,
                                        uint8_t value) {
  return m_core->SetPortPriorityOverride(device_alias, port, is_output, value);
}

/*
 * Sends a device config request
 *
 * @param device_id the device id
 * @param msg  the request message
 * @return true on success, false on failure
 */
bool OlaClient::ConfigureDevice(unsigned int device_id, const string &msg) {
  return m_core->ConfigureDevice(device_id, msg);
}
}  // ola
