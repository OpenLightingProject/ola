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
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "plugins/usbpro/DMXter4Device.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::string;
using ola::network::NetworkToHost;
using ola::rdm::UID;


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
    UsbDevice(owner, name, widget),
    m_port(NULL) {
  std::stringstream str;
  str << std::hex << esta_id << "-" << device_id << "-" <<
    NetworkToHost(serial);
  m_device_id = str.str();

  m_port = new DMXter4DeviceOutputPort(this);
  AddPort(m_port);

  widget->SetMessageHandler(this);
  Start();
  (void) plugin_adaptor;
}


/**
 * Clean up
 */
DMXter4Device::~DMXter4Device() {
}


/**
 * Called after we start, use this to fetch the TOD.
 */
bool DMXter4Device::StartHook() {
  SendTodRequest();
  return true;
}


/**
 * Called when a new packet arrives
 */
void DMXter4Device::HandleMessage(UsbWidget *widget,
                                  uint8_t label,
                                  unsigned int length,
                                  const uint8_t *data) {
  OLA_INFO << "Got new packet: 0x" << std::hex << (int) label <<
    ", size " << length;

  switch (label) {
    case TOD_LABEL:
      HandleTodResponse(length, data);
      break;
    default:
      OLA_WARN << "Unknown label: 0x" << std::hex << (int) label;
  }
  return;
  (void) widget;
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
 * Notify the port that there are new UIDs
 */
void DMXter4Device::SendUIDUpdate() {
  m_port->NewUIDList(m_uids);
}


/**
 * Send a TOD request to the widget
 */
void DMXter4Device::SendTodRequest() {
  m_widget->SendMessage(TOD_LABEL, 0, NULL);
  OLA_INFO << "Sent TOD request";
}


/**
 * Handle a TOD response
 */
void DMXter4Device::HandleTodResponse(unsigned int length,
                                      const uint8_t *data) {
  (void) data;
  if (length % UID::UID_SIZE) {
    OLA_WARN << "Response length " << length << " not divisible by " <<
      (int) ola::rdm::UID::UID_SIZE << ", ignoring packet";
    return;
  }

  m_uids.Clear();
  for (unsigned int i = 0; i < length; i+= UID::UID_SIZE) {
    UID uid(data + i);
    OLA_INFO << "added " << uid.ToString();
    m_uids.AddUID(uid);
  }
  SendUIDUpdate();
}
}  // usbpro
}  // plugin
}  // ola
