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
 * DmxterDevice.h
 * The Ardunio RGB Mixer device.
 * Copyright (C) 2010 Simon Newton
 */

#include <algorithm>
#include <string>
#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DmxterDevice.h"
#include "plugins/usbpro/DmxterWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;
using ola::network::NetworkToHost;
using ola::rdm::UID;


/*
 * New Arduino RGB Device
 */
DmxterDevice::DmxterDevice(ola::network::SelectServerInterface *ss,
                             ola::AbstractPlugin *owner,
                             const string &name,
                             UsbWidget *widget,
                             uint16_t esta_id,
                             uint16_t device_id,
                             uint32_t serial):
    UsbDevice(owner, name, widget),
    m_dmxter_widget(NULL) {
  std::stringstream str;
  str << std::hex << esta_id << "-" << device_id << "-" <<
    NetworkToHost(serial);
  m_device_id = str.str();

  m_dmxter_widget = new DmxterWidget(ss, widget, esta_id, serial);

  ola::BasicOutputPort *port = new DmxterOutputPort(this);
  AddPort(port);

  m_dmxter_widget->SetUIDListCallback(
      ola::NewCallback(port, &DmxterOutputPort::NewUIDList));
}


/**
 * Clean up
 */
DmxterDevice::~DmxterDevice() {
  delete m_dmxter_widget;
}


/**
 * Called after we start, use this to fetch the TOD.
 */
bool DmxterDevice::StartHook() {
  m_dmxter_widget->SendTodRequest();
  return true;
}


/**
 *
 */
void DmxterDevice::HandleRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  m_dmxter_widget->SendRDMRequest(request, callback);
}

/**
 *
 */
void DmxterDevice::RunRDMDiscovery() {
}


/**
 * Notify the port that there are new UIDs
 */
void DmxterDevice::SendUIDUpdate() {
  m_dmxter_widget->SendUIDUpdate();
}
}  // usbpro
}  // plugin
}  // ola
