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
 * Siudi.cpp
 * The synchronous SIUDI widgets.
 * Copyright (C) 2023 Alexander Simon
 */

#include "plugins/usbdmx/Siudi.h"

#include <string.h>
#include <unistd.h>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Constants.h"
#include "ola/Logging.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;

namespace {

static const uint8_t ENDPOINT = 2;
static const unsigned int TIMEOUT = 50; // 50ms is ok
enum {SIUDI_PACKET_SIZE = 512};

}  // namespace

// SiudiThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a SIUDI device in a separate thread.
 */
class SiudiThreadedSender: public ThreadedUsbSender {
 public:
  SiudiThreadedSender(LibUsbAdaptor *adaptor,
                        libusb_device *usb_device,
                        libusb_device_handle *handle);

  bool Start();

private:
  LibUsbAdaptor* const m_adaptor;
  uint8_t m_packet[SIUDI_PACKET_SIZE];
  libusb_device_handle* const m_usb_handle;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

SiudiThreadedSender::SiudiThreadedSender(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    libusb_device_handle *usb_handle)
    : ThreadedUsbSender(usb_device, usb_handle),
      m_adaptor(adaptor), m_usb_handle(usb_handle) {
  memset(m_packet, 0x00, SIUDI_PACKET_SIZE);
}

bool SiudiThreadedSender::Start() {
  if (!ThreadedUsbSender::Start()) {
    return false;
  }

  // Read device info. This call takes about 270 ms.
  // Discard the buffer as the format is currently unknown.
  uint8_t buf[64];
  int ret = libusb_control_transfer(m_usb_handle,
                                LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN,
                                0x3f, 0x00c4, 1, buf, 64, 500);
  if (ret != 64) {
    OLA_WARN << "Failed to read SIUDI information: "
            << (ret < 0 ? LibUsbAdaptor::ErrorCodeToString(ret) : "Short read");
    return false;
  }

  // Unstall the endpoint. The original software seems to call this regularly.
  ret = libusb_clear_halt(m_usb_handle, ENDPOINT);
  if (ret != 0) {
    OLA_WARN << "Failed to reset SIUDI endpoint: "
            << (ret < 0 ? LibUsbAdaptor::ErrorCodeToString(ret) : "Unknown");
    return false;
  }
  usleep(10000);

  return true;
}

bool SiudiThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                           const DmxBuffer &buffer) {
  for (unsigned int i = 0; i < buffer.Size(); i++) {
    m_packet[i] = buffer.Get(i);
  }
  int transferred;
  int r = m_adaptor->BulkTransfer(
    handle, ENDPOINT, (unsigned char*) m_packet,
    SIUDI_PACKET_SIZE, &transferred, TIMEOUT);
  if (transferred != SIUDI_PACKET_SIZE) {
    // not sure if this is fatal or not
    OLA_WARN << "SIUDI driver failed to transfer all data";
  }
  usleep(20000);
  return r == 0;
}

// SynchronousSiudi
// -----------------------------------------------------------------------------

SynchronousSiudi::SynchronousSiudi(LibUsbAdaptor *adaptor,
                                                   libusb_device *usb_device)
    : Siudi(adaptor, usb_device) {
}

bool SynchronousSiudi::Init() {
  libusb_device_handle *usb_handle;

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, 0, &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<SiudiThreadedSender> sender(
      new SiudiThreadedSender(m_adaptor, m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousSiudi::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
