/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  OlaConfigurator.cpp
 *  Makes configuring devices easy
 *  Copyright (C) 2005-2009 Simon Newton
 */

#include <ola/Callback.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

#include "examples/OlaConfigurator.h"

using ola::NewSingleCallback;
using ola::OlaCallbackClient;
using ola::OlaDevice;
using ola::network::SelectServer;
using std::cout;
using std::endl;
using std::string;
using std::vector;


/*
 * Clean up
 */
OlaConfigurator::~OlaConfigurator() {
  delete m_client_wrapper;
}


/*
 * Setup the configurator
 * @return true on success, false on failure
 */
bool OlaConfigurator::Setup() {
  m_client_wrapper = new ola::OlaCallbackClientWrapper();
  if (!m_client_wrapper->Setup()) {
    delete m_client_wrapper;
    m_client_wrapper = NULL;
    return false;
  }

  m_client = m_client_wrapper->GetClient();
  m_ss = m_client_wrapper->GetSelectServer();

  // fire off a DeviceInfo request
  m_client->FetchDeviceInfo(
      m_plugin_id,
      NewSingleCallback(this, &OlaConfigurator::HandleDevices));
  return true;
}


/*
 * Send a ConfigureDevice() request
 * @param message the request to send
 */
bool OlaConfigurator::SendMessage(const google::protobuf::Message &message) {
  string request_string;
  message.SerializeToString(&request_string);
  return m_client->ConfigureDevice(
      m_alias,
      request_string,
      NewSingleCallback(this, &OlaConfigurator::HandleConfigResponse));
}


/*
 * Handle the DeviceInfo response. We do this to ensure that the plugin this
 * device corresponds to is the one we expect.
 *
 * @param devices a vector of OlaDevice objects
 * @param error an error string
 */
void OlaConfigurator::HandleDevices(const vector <OlaDevice> &devices,
                                    const string &error) {
  if (!error.empty()) {
    cout << "Error: " << error << endl;
    m_ss->Terminate();
    return;
  }

  vector <OlaDevice>::const_iterator iter;
  for (iter = devices.begin(); iter != devices.end(); ++iter) {
    if (iter->Alias() == m_alias && iter->PluginId() == m_plugin_id) {
      SendConfigRequest();
      return;
    }
  }
  cout << "Device " << m_alias << " is of the wrong type or missing." << endl;
  m_ss->Terminate();
}
