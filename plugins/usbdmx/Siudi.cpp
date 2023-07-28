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
// SIUDI-6 blocks USB transfers during an ongoing DMX TX.
// One package needs about 32 ms to be sent.
// Wait 30 ms between two USB bulk transfers and expect 2 ms USB response delay.
static const unsigned int BULK_TIMEOUT = 10;
static const unsigned int BULK_DELAY = (30 * 1000);
static const unsigned int CONTROL_TIMEOUT = 500;
static const unsigned int DEVINFO_REQUEST = 0x3f;
static const unsigned int DEVINFO_SIZE = 64;

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
}

bool SiudiThreadedSender::Start() {
  if (!ThreadedUsbSender::Start()) {
    return false;
  }

  // Read device info. This call takes about 270 ms.
  // Discard the buffer as the format is currently unknown.
  uint8_t buf[DEVINFO_SIZE];
  int ret = libusb_control_transfer(m_usb_handle,
      LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN,
      DEVINFO_REQUEST, 0x0000, 1, buf, DEVINFO_SIZE, CONTROL_TIMEOUT);
  if (ret != DEVINFO_SIZE) {
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
  usleep(BULK_DELAY);  // Might receive errors if writing too early.

  return true;
}

bool SiudiThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                           const DmxBuffer &buffer) {
  int transferred, r;
  unsigned int buf_size = buffer.Size();
  if (buf_size == ola::DMX_UNIVERSE_SIZE) {
    // As we are sending, we can cast the const buffer to a writeable pointer.
    r = m_adaptor->BulkTransfer(
      handle, ENDPOINT, (unsigned char*)buffer.GetRaw(),
      ola::DMX_UNIVERSE_SIZE, &transferred, BULK_TIMEOUT);
  } else {
    unsigned char buf[buf_size];
    unsigned int buf_get_size = ola::DMX_UNIVERSE_SIZE;
    buffer.GetRange(0, buf, &buf_get_size);
    if (buf_get_size < ola::DMX_UNIVERSE_SIZE) {
      memset(&buf[buf_get_size], 0x00, ola::DMX_UNIVERSE_SIZE - buf_get_size);
    }
    r = m_adaptor->BulkTransfer(
      handle, ENDPOINT, buf, ola::DMX_UNIVERSE_SIZE, &transferred, BULK_TIMEOUT);
  }
  if (transferred != ola::DMX_UNIVERSE_SIZE) {
    // not sure if this is fatal or not
    OLA_WARN << "SIUDI driver failed to transfer all data";
  }
  usleep(BULK_DELAY);
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
