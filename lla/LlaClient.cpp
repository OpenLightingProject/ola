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
 * LlaClient.cpp
 * Implementation of LlaClient
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <lla/LlaClient.h>
#include "LlaClientCore.h"

namespace lla {

using lla::network::ConnectedSocket;

LlaClient::LlaClient(ConnectedSocket *socket) {
  m_core = new LlaClientCore(socket);
}

LlaClient::~LlaClient() {
  delete m_core;
}


/*
 * Setup this client
 *
 * @returns true on success, false on failure
 */
bool LlaClient::Setup() {
  return m_core->Setup();
}


/*
 * Close the lla connection.
 *
 * @return true on sucess, false on failure
 */
bool LlaClient::Stop() {
  return m_core->Stop();
}


/*
 * Set the LlaClientObserver object
 *
 * @params observer the LlaClientObserver object to be used for the callbacks.
 */
bool LlaClient::SetObserver(LlaClientObserver *observer) {
  return m_core->SetObserver(observer);
}


/*
 * Fetch info about available plugins. This results in a call to
 *   observer->Plugins(...)
 *  when the request returns.
 *
 * @params plugin_id use this to filter on plugin id
 * @params include_description set to true to get the plugin descriptions as
 * well.
 * @returns true if the request succeeded, false otherwise.
 */
bool LlaClient::FetchPluginInfo(int plugin_id, bool include_description) {
  return m_core->FetchPluginInfo(plugin_id, include_description);
}


/*
 * Write some dmx data.
 *
 * @param universe   universe to send to
 * @param data  dmx data
 * @param length  length of dmx data
 * @return true on success, false on failure
 */
bool LlaClient::SendDmx(unsigned int universe, dmx_t *data, unsigned int length) {
  return m_core->SendDmx(universe, data, length);
}


/*
 * Read dmx data. This results in a call to
 *  observer->NewDmx()
 * when the request returns.
 *
 * @param universe the universe id to get data for
 * @return true on success, false on failure
 */
bool LlaClient::FetchDmx(unsigned int universe) {
  return m_core->FetchDmx(universe);
}


/*
 * Request a listing of what devices are attached. This results in a call to
 *   observer->Devices()
 * when the request returns.
 *
 * @param filter only fetch devices that belong to this plugin
 * @return true on success, false on failure
 */
bool LlaClient::FetchDeviceInfo(lla_plugin_id filter) {
  return m_core->FetchDeviceInfo(filter);
}


/*
 * Request information about active universes. This results in a call to
 *   observer->Universes()
 * when the request returns.
 *
 * @return true on success, false on failure
 */
bool LlaClient::FetchUniverseInfo() {
  return m_core->FetchUniverseInfo();
}


/*
 * Set the name of a universe.
 *
 * @return true on success, false on failure
 */
bool LlaClient::SetUniverseName(unsigned int universe, const string &name) {
  return m_core->SetUniverseName(universe, name);
}


/*
 * Set the merge mode of a universe
 *
 * @return true on success, false on failure
 */
bool LlaClient::SetUniverseMergeMode(unsigned int universe,
                                     LlaUniverse::merge_mode mode) {
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
bool LlaClient::RegisterUniverse(unsigned int universe,
                                 lla::RegisterAction register_action) {
  return m_core->RegisterUniverse(universe, register_action);
}


/*
 * (Un)Patch a port to a universe
 *
 * @param dev     the device id
 * @param port    the port id
 * @param action  LlaClient::PATCH or LlaClient::UNPATCH
 * @param uni    universe id
 * @return true on success, false on failure
 */
bool LlaClient::Patch(unsigned int device_id,
                      unsigned int port_id,
                      lla::PatchAction patch_action,
                      unsigned int universe) {
  return m_core->Patch(device_id, port_id, patch_action, universe);
}


/*
 * Sends a device config request
 *
 * @param device_id the device id
 * @param msg  the request message
 * @return true on success, false on failure
 */
bool LlaClient::ConfigureDevice(unsigned int device_id, const string &msg) {
  return m_core->ConfigureDevice(device_id, msg);
}

} // lla
