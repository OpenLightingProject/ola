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
 * USBDMXCom.cpp
 * The synchronous and asynchronous USBDMXCom widgets.
 * Copyright (C) 2021 Peter Newman
 */

#include "plugins/usbdmx/USBDMXCom.h"

#include <string.h>
#include <string>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/util/Utils.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;
using ola::usb::LibUsbAdaptor;

namespace {

// Why is this so long?
static const unsigned int URB_TIMEOUT_MS = 500;
static const unsigned char ENDPOINT = 0x02;
// TODO(Someone): Set a sane value here
enum { USBDMXCOM_FRAME_SIZE = 100 };

/*
 * Create a USBDMX.com message to match the supplied DmxBuffer.
 */
void CreateFrame(
    const DmxBuffer &buffer,
    uint8_t frame[USBDMXCOM_FRAME_SIZE]) {
  unsigned int frame_size = buffer.Size();

  // header
  // LSB first.
  //utils::SplitUInt16(DMX_UNIVERSE_SIZE + 1, &frame[3], &frame[2]);
  buffer.Get(frame + 5, &frame_size);
  memset(frame + 5 + frame_size, 0, DMX_UNIVERSE_SIZE - frame_size);
  // End message delimiter
}

/*
 * Find the interface with the endpoint we're after. Usually this is interface
 * 1 but we check them all just in case.
 */
bool LocateInterface(LibUsbAdaptor *adaptor,
                     libusb_device *usb_device,
                     int *interface_number) {
  struct libusb_config_descriptor *device_config;
  if (adaptor->GetConfigDescriptor(usb_device, 0, &device_config) != 0) {
    OLA_WARN << "Failed to get device config descriptor";
    return false;
  }

  OLA_DEBUG << static_cast<int>(device_config->bNumInterfaces)
            << " interfaces found";
  for (unsigned int i = 0; i < device_config->bNumInterfaces; i++) {
    const struct libusb_interface *interface = &device_config->interface[i];
    for (int j = 0; j < interface->num_altsetting; j++) {
      const struct libusb_interface_descriptor *iface_descriptor =
          &interface->altsetting[j];
      for (uint8_t k = 0; k < iface_descriptor->bNumEndpoints; k++) {
        const struct libusb_endpoint_descriptor *endpoint =
            &iface_descriptor->endpoint[k];
        OLA_DEBUG << "Interface " << i << ", altsetting " << j << ", endpoint "
                  << static_cast<int>(k) << ", endpoint address "
                  << ola::strings::ToHex(endpoint->bEndpointAddress);
        if (endpoint->bEndpointAddress == ENDPOINT) {
          OLA_INFO << "Using interface " << i;
          *interface_number = i;
          adaptor->FreeConfigDescriptor(device_config);
          return true;
        }
      }
    }
  }
  OLA_WARN << "Failed to locate endpoint for USBDMXCom device.";
  adaptor->FreeConfigDescriptor(device_config);
  return false;
}
}  // namespace

// USBDMXComThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a USBDMXCom device in a separate thread.
 */
class USBDMXComThreadedSender: public ThreadedUsbSender {
 public:
  USBDMXComThreadedSender(LibUsbAdaptor *adaptor,
                          libusb_device *usb_device,
                          libusb_device_handle *handle);

 private:
  LibUsbAdaptor* const m_adaptor;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

USBDMXComThreadedSender::USBDMXComThreadedSender(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    libusb_device_handle *usb_handle)
    : ThreadedUsbSender(usb_device, usb_handle),
      m_adaptor(adaptor) {
}

bool USBDMXComThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                               const DmxBuffer &buffer) {
  uint8_t frame[USBDMXCOM_FRAME_SIZE];
  CreateFrame(buffer, frame);

  int transferred;
  int r = m_adaptor->BulkTransfer(handle, ENDPOINT, frame,
                                  USBDMXCOM_FRAME_SIZE, &transferred,
                                  URB_TIMEOUT_MS);
  if (transferred != USBDMXCOM_FRAME_SIZE) {
    // not sure if this is fatal or not
    OLA_WARN << "USBDMXCom driver failed to transfer all data";
  }
  return (r == 0);
}

// SynchronousUSBDMXCom
// -----------------------------------------------------------------------------

SynchronousUSBDMXCom::SynchronousUSBDMXCom(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const string &serial)
    : USBDMXCom(adaptor, usb_device, serial) {
}

bool SynchronousUSBDMXCom::Init() {
  libusb_device_handle *usb_handle;

  int interface_number;
  if (!LocateInterface(m_adaptor, m_usb_device, &interface_number)) {
    return false;
  }

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, interface_number, &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<USBDMXComThreadedSender> sender(
      new USBDMXComThreadedSender(m_adaptor, m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousUSBDMXCom::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// USBDMXComAsyncUsbSender
// -----------------------------------------------------------------------------
class USBDMXComAsyncUsbSender : public AsyncUsbSender {
 public:
  USBDMXComAsyncUsbSender(LibUsbAdaptor *adaptor,
                          libusb_device *usb_device)
      : AsyncUsbSender(adaptor, usb_device) {
  }

  ~USBDMXComAsyncUsbSender() {
    CancelTransfer();
  }

  libusb_device_handle* SetupHandle() {
    int interface_number;
    if (!LocateInterface(m_adaptor, m_usb_device, &interface_number)) {
      return NULL;
    }

    libusb_device_handle *usb_handle;
    bool ok = m_adaptor->OpenDeviceAndClaimInterface(
        m_usb_device, interface_number, &usb_handle);
    if (!ok) {
      return NULL;
    }

    return usb_handle;
  }

  bool PerformTransfer(const DmxBuffer &buffer) {
    CreateFrame(buffer, m_tx_frame);
    FillBulkTransfer(ENDPOINT, m_tx_frame, USBDMXCOM_FRAME_SIZE,
                     URB_TIMEOUT_MS);
    return (SubmitTransfer() == 0);
  }

 private:
  uint8_t m_tx_frame[USBDMXCOM_FRAME_SIZE];

  DISALLOW_COPY_AND_ASSIGN(USBDMXComAsyncUsbSender);
};

// AsynchronousUSBDMXCom
// -----------------------------------------------------------------------------

AsynchronousUSBDMXCom::AsynchronousUSBDMXCom(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const string &serial)
    : USBDMXCom(adaptor, usb_device, serial) {
  m_sender.reset(new USBDMXComAsyncUsbSender(m_adaptor,
                                             usb_device));
}

bool AsynchronousUSBDMXCom::Init() {
  return m_sender->Init();
}

bool AsynchronousUSBDMXCom::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
