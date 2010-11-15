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
 * DMXter4Device.h
 * The Ardunio RGB Mixer device.
 * Copyright (C) 2010 Simon Newton
 */

#include <algorithm>
#include <string>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/DMXter4Device.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;
using ola::network::NetworkToHost;


/*
 * New Arduino RGB Device
 */
DMXter4Device::DMXter4Device(const ola::PluginAdaptor *plugin_adaptor,
                             ola::AbstractPlugin *owner,
                             const string &name,
                             UsbWidget *widget,
                             uint16_t esta_id,
                             uint16_t device_id,
                             uint32_t serial):
    UsbDevice(owner, name, widget) {
  std::stringstream str;
  str << std::hex << esta_id << "-" << device_id << "-" <<
    NetworkToHost(serial);
  m_device_id = str.str();

  OutputPort *output_port = new DMXter4DeviceOutputPort(this);
  AddPort(output_port);
  Start();
  (void) plugin_adaptor;
}


/**
 *
 */
bool DMXter4Device::HandleRDMRequest(const ola::rdm::RDMRequest *request) {
  OLA_WARN << "RDM not implemented";
  delete request;
  return true;
}

/**
 *
 */
void DMXter4Device::RunRDMDiscovery() {
}

/**
 *
 */
void DMXter4Device::SendUIDUpdate() {
}
}  // usbpro
}  // plugin
}  // ola
