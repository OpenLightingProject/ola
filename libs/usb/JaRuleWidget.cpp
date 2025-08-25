/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * JaRuleWidget.cpp
 * A Ja Rule widget.
 * Copyright (C) 2015 Simon Newton
 */

#include "libs/usb/JaRuleWidget.h"

#include <ola/Callback.h>
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/base/Array.h>
#include <ola/stl/STLUtils.h>
#include <ola/strings/Format.h>
#include <ola/thread/Mutex.h>
#include <ola/util/Utils.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "libs/usb/JaRulePortHandle.h"
#include "libs/usb/JaRuleWidgetPort.h"

namespace ola {
namespace usb {

using ola::io::ByteString;
using ola::rdm::UID;
using std::unique_ptr;
using std::string;

namespace {

struct EndpointCapabilties {
  EndpointCapabilties()
    : in_supported(false),
      out_supported(false),
      in_interface(0),
      out_interface(0) {}

  bool in_supported;
  bool out_supported;
  int in_interface;
  int out_interface;
};

}  // namespace

JaRuleWidget::JaRuleWidget(ola::thread::ExecutorInterface *executor,
                           AsynchronousLibUsbAdaptor *adaptor,
                           libusb_device *usb_device)
  : m_executor(executor),
    m_adaptor(adaptor),
    m_device(usb_device),
    m_usb_handle(NULL),
    m_uid(0, 0) {
  m_adaptor->RefDevice(m_device);
}

JaRuleWidget::~JaRuleWidget() {
  STLDeleteElements(&m_ports);

  if (m_usb_handle) {
    m_adaptor->Close(m_usb_handle);
  }

  m_adaptor->UnrefDevice(m_device);
}

bool JaRuleWidget::Init() {
  bool ok = InternalInit();
  if (!ok) {
    STLDeleteElements(&m_ports);
    if (m_usb_handle) {
      m_adaptor->Close(m_usb_handle);
      m_usb_handle = NULL;
    }
  }
  return ok;
}

USBDeviceID JaRuleWidget::GetDeviceId() const {
  return m_adaptor->GetDeviceId(m_device);
}

void JaRuleWidget::CancelAll(uint8_t port_index) {
  if (port_index > m_ports.size() - 1) {
    return;
  }
  JaRuleWidgetPort *port_info = m_ports[port_index];
  port_info->CancelAll();
}

uint8_t JaRuleWidget::PortCount() const {
  return m_ports.size();
}

UID JaRuleWidget::GetUID() const {
  return m_uid;
}

string JaRuleWidget::ManufacturerString() const {
  return m_manufacturer;
}

string JaRuleWidget::ProductString() const {
  return m_product;
}

JaRulePortHandle* JaRuleWidget::ClaimPort(uint8_t port_index) {
  if (port_index > m_ports.size() - 1) {
    return NULL;
  }
  return m_ports[port_index]->ClaimPort();
}

void JaRuleWidget::ReleasePort(uint8_t port_index) {
  if (port_index > m_ports.size() - 1) {
    return;
  }
  m_ports[port_index]->ReleasePort();
}

void JaRuleWidget::SendCommand(uint8_t port_index,
                               CommandClass command,
                               const uint8_t *data,
                               unsigned int size,
                               CommandCompleteCallback *callback) {
  if (port_index > m_ports.size() - 1) {
    OLA_WARN << "Invalid JaRule Port " << static_cast<int>(port_index);
    if (callback) {
      callback->Run(COMMAND_RESULT_INVALID_PORT, RC_UNKNOWN, 0, ByteString());
    }
    return;
  }
  m_ports[port_index]->SendCommand(command, data, size, callback);
}

// Private Methods
// ----------------------------------------------------------------------------

bool JaRuleWidget::InternalInit() {
  struct libusb_config_descriptor *config;
  int error = m_adaptor->GetActiveConfigDescriptor(m_device, &config);
  if (error) {
    return false;
  }

  // Each endpoint address is 8 bits. Bit 7 is the endpoint direction (in/out).
  // The lower 4 bits are the endpoint number. We try to find bulk endpoints
  // with matching numbers.
  typedef std::map<uint8_t, EndpointCapabilties> EndpointMap;
  EndpointMap endpoint_map;

  for (uint8_t iface_index = 0; iface_index < config->bNumInterfaces;
       iface_index++) {
    const struct libusb_interface &iface = config->interface[iface_index];
    // We don't support alt settings.
    if (iface.num_altsetting != 1) {
      continue;
    }
    const struct libusb_interface_descriptor &iface_descriptor =
        iface.altsetting[0];
    if (iface_descriptor.bInterfaceClass == LIBUSB_CLASS_VENDOR_SPEC &&
        iface_descriptor.bInterfaceSubClass == SUBCLASS_VALUE &&
        iface_descriptor.bInterfaceProtocol == PROTOCOL_VALUE) {
      // Vendor class, subclass & protocol
      for (uint8_t endpoint_index = 0; endpoint_index <
           iface_descriptor.bNumEndpoints; endpoint_index++) {
        const struct libusb_endpoint_descriptor &endpoint =
            iface_descriptor.endpoint[endpoint_index];
        if ((endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) !=
             LIBUSB_TRANSFER_TYPE_BULK) {
          continue;
        }

        uint8_t endpoint_address = endpoint.bEndpointAddress;
        uint8_t endpoint_number = (
            endpoint_address & LIBUSB_ENDPOINT_ADDRESS_MASK);
        uint8_t endpoint_direction = (
            endpoint_address & LIBUSB_ENDPOINT_DIR_MASK);

        if (endpoint_direction == LIBUSB_ENDPOINT_IN) {
          endpoint_map[endpoint_number].in_supported = true;
          endpoint_map[endpoint_number].in_interface = iface_index;
        }
        if (endpoint_direction == LIBUSB_ENDPOINT_OUT) {
          endpoint_map[endpoint_number].out_supported = true;
          endpoint_map[endpoint_number].out_interface = iface_index;
        }
      }
    }
  }

  m_adaptor->FreeConfigDescriptor(config);

  if (!m_adaptor->OpenDevice(m_device, &m_usb_handle)) {
    return false;
  }

  // Get the serial number (UID) of the device.
  libusb_device_descriptor device_descriptor;
  if (m_adaptor->GetDeviceDescriptor(m_device, &device_descriptor)) {
    return false;
  }

  AsynchronousLibUsbAdaptor::DeviceInformation device_info;
  if (!m_adaptor->GetDeviceInfo(m_device, device_descriptor, &device_info)) {
    return false;
  }

  unique_ptr<UID> uid(UID::FromString(device_info.serial));
  if (!uid.get() || uid->IsBroadcast()) {
    OLA_WARN << "Invalid JaRule serial number: " << device_info.serial;
    return false;
  }

  m_uid = *uid;
  m_manufacturer = device_info.manufacturer;
  m_product = device_info.product;

  std::set<int> interfaces_to_claim;

  EndpointMap::const_iterator endpoint_iter = endpoint_map.begin();
  uint8_t port_index = 0;
  for (; endpoint_iter != endpoint_map.end(); ++endpoint_iter) {
    const EndpointCapabilties &capabilities = endpoint_iter->second;
    if (capabilities.in_supported && capabilities.out_supported) {
      interfaces_to_claim.insert(capabilities.in_interface);
      interfaces_to_claim.insert(capabilities.out_interface);
      OLA_INFO << "Found Ja Rule port at "
               << static_cast<int>(endpoint_iter->first);
      m_ports.push_back(new JaRuleWidgetPort(
            m_executor, m_adaptor, m_usb_handle, endpoint_iter->first,
            m_uid, port_index++));
    }
  }

  std::set<int>::const_iterator iface_iter = interfaces_to_claim.begin();
  for (; iface_iter != interfaces_to_claim.end(); ++iface_iter) {
    if (m_adaptor->ClaimInterface(m_usb_handle, *iface_iter)) {
      return false;
    }
  }

  OLA_INFO << "Found JaRule device : " << m_uid;
  return true;
}
}  // namespace usb
}  // namespace ola
