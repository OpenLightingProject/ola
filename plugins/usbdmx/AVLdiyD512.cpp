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
 * AVLdiyD512.cpp
 * The synchronous and asynchronous AVLdiy D512 widgets.
 * Copyright (C) 2016 Tor Håkon Gjerde
 */

#include "plugins/usbdmx/AVLdiyD512.h"

#include <unistd.h>
#include <string>
#include <utility>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/Constants.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;
using ola::usb::LibUsbAdaptor;

namespace {

static const unsigned int URB_TIMEOUT_MS = 500;
static const unsigned int UDMX_SET_CHANNEL_RANGE = 0x0002;

}  // namespace

// AVLdiyThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a AVLdiy device in a separate thread.
 */
class AVLdiyThreadedSender: public ThreadedUsbSender {
 public:
  AVLdiyThreadedSender(LibUsbAdaptor *adaptor,
                       libusb_device *usb_device,
                       libusb_device_handle *handle)
      : ThreadedUsbSender(usb_device, handle),
        m_adaptor(adaptor) {
  }

 private:
  LibUsbAdaptor* const m_adaptor;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

bool AVLdiyThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                          const DmxBuffer &buffer) {
  int r = m_adaptor->ControlTransfer(
      handle,
      LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
      LIBUSB_ENDPOINT_OUT,  // bmRequestType
      UDMX_SET_CHANNEL_RANGE,  // bRequest
      buffer.Size(),  // wValue
      0,  // wIndex
      const_cast<unsigned char*>(buffer.GetRaw()),  // data
      buffer.Size(),  // wLength
      URB_TIMEOUT_MS);  // timeout
  // Sometimes we get PIPE errors here, those are non-fatal
  return r > 0 || r == LIBUSB_ERROR_PIPE;
}


// SynchronousAVLdiyD512
// -----------------------------------------------------------------------------

SynchronousAVLdiyD512::SynchronousAVLdiyD512(LibUsbAdaptor *adaptor,
                                             libusb_device *usb_device,
                                             const string &serial)
    : AVLdiyD512(adaptor, usb_device, serial) {
}

bool SynchronousAVLdiyD512::Init() {
  libusb_device_handle *usb_handle;

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, 0, &usb_handle);
  if (!ok) {
    return false;
  }

  std::unique_ptr<AVLdiyThreadedSender> sender(
      new AVLdiyThreadedSender(m_adaptor, m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender = std::move(sender);
  return true;
}

bool SynchronousAVLdiyD512::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// AVLdiyAsyncUsbSender
// -----------------------------------------------------------------------------
class AVLdiyAsyncUsbSender : public AsyncUsbSender {
 public:
  AVLdiyAsyncUsbSender(LibUsbAdaptor *adaptor, libusb_device *usb_device)
      : AsyncUsbSender(adaptor, usb_device) {
    m_control_setup_buffer =
        new uint8_t[LIBUSB_CONTROL_SETUP_SIZE + DMX_UNIVERSE_SIZE];
  }

  ~AVLdiyAsyncUsbSender() {
    CancelTransfer();
    delete[] m_control_setup_buffer;
  }

  libusb_device_handle* SetupHandle() {
    libusb_device_handle *usb_handle;
    bool ok = m_adaptor->OpenDeviceAndClaimInterface(
        m_usb_device, 0, &usb_handle);
    return ok ? usb_handle : NULL;
  }

  bool PerformTransfer(const DmxBuffer &buffer) {
    m_adaptor->FillControlSetup(
        m_control_setup_buffer,
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
        LIBUSB_ENDPOINT_OUT,  // bmRequestType
        UDMX_SET_CHANNEL_RANGE,  // bRequest
        buffer.Size(),  // wValue
        0,  // wIndex
        buffer.Size());  // wLength

    unsigned int length = DMX_UNIVERSE_SIZE;
    buffer.Get(m_control_setup_buffer + LIBUSB_CONTROL_SETUP_SIZE, &length);

    FillControlTransfer(m_control_setup_buffer, URB_TIMEOUT_MS);
    return (SubmitTransfer() == 0);
  }

 private:
  uint8_t *m_control_setup_buffer;

  DISALLOW_COPY_AND_ASSIGN(AVLdiyAsyncUsbSender);
};

// AsynchronousAVLdiyD512
// -----------------------------------------------------------------------------

AsynchronousAVLdiyD512::AsynchronousAVLdiyD512(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const string &serial)
    : AVLdiyD512(adaptor, usb_device, serial) {
  m_sender.reset(new AVLdiyAsyncUsbSender(m_adaptor, usb_device));
}

bool AsynchronousAVLdiyD512::Init() {
  return m_sender->Init();
}

bool AsynchronousAVLdiyD512::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
