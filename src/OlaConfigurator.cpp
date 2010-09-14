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

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>

#include "src/OlaConfigurator.h"

using ola::OlaClient;
using ola::OlaDevice;
using ola::SimpleClient;
using ola::network::SelectServer;
using std::cout;
using std::endl;
using std::string;
using std::vector;


void Observer::DeviceConfig(unsigned int device_alias,
                            const string &reply,
                            const string &error) {
  m_configurator->HandleConfigResponse(reply, error);
}


void Observer::Devices(const vector <OlaDevice> &devices,
                       const string &error) {
  m_configurator->HandleDevices(devices, error);
}


/*
 * Clean up
 */
OlaConfigurator::~OlaConfigurator() {
  delete m_simple_client;
  delete m_observer;
}


/*
 * Setup the configurator
 * @return true on success, false on failure
 */
bool OlaConfigurator::Setup() {
  m_simple_client = new SimpleClient();
  if (!m_simple_client->Setup()) {
    delete m_simple_client;
    m_simple_client = NULL;
    return false;
  }

  m_client = m_simple_client->GetClient();
  m_ss = m_simple_client->GetSelectServer();
  m_observer = new Observer(this);
  m_client->SetObserver(m_observer);

  // fire off a DeviceInfo request
  m_client->FetchDeviceInfo(m_plugin_id);
  return true;
}


/*
 * Send a ConfigureDevice() request
 * @param message the request to send
 */
bool OlaConfigurator::SendMessage(const google::protobuf::Message &message) {
  string request_string;
  message.SerializeToString(&request_string);
  return m_client->ConfigureDevice(m_alias, request_string);
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
