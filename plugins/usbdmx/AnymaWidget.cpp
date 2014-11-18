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
 * AnymaWidget.cpp
 * The synchronous and asynchronous Anyma widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/AnymaWidget.h"

#include <unistd.h>
#include <string>

#include "ola/Logging.h"
#include "ola/Constants.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/LibUsbAdaptor.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

namespace {

static const unsigned int URB_TIMEOUT_MS = 500;
static const unsigned int UDMX_SET_CHANNEL_RANGE = 0x0002;

}  // namespace

// AnymaThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a Anyma device in a separate thread.
 */
class AnymaThreadedSender: public ThreadedUsbSender {
 public:
  AnymaThreadedSender(libusb_device *usb_device,
                      libusb_device_handle *handle);

 private:
  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

AnymaThreadedSender::AnymaThreadedSender(
    libusb_device *usb_device,
    libusb_device_handle *usb_handle)
    : ThreadedUsbSender(usb_device, usb_handle) {
}

bool AnymaThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                         const DmxBuffer &buffer) {
  int r = libusb_control_transfer(
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


// SynchronousAnymaWidget
// -----------------------------------------------------------------------------

SynchronousAnymaWidget::SynchronousAnymaWidget(LibUsbAdaptor *adaptor,
                                               libusb_device *usb_device,
                                               const string &serial)
    : AnymaWidget(adaptor, serial),
      m_usb_device(usb_device) {
}

bool SynchronousAnymaWidget::Init() {
  libusb_device_handle *usb_handle;

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, 0, &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<AnymaThreadedSender> sender(
      new AnymaThreadedSender(m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousAnymaWidget::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// AnymaAsyncUsbSender
// -----------------------------------------------------------------------------
class AnymaAsyncUsbSender : public AsyncUsbSender {
 public:
  AnymaAsyncUsbSender(LibUsbAdaptor *adaptor,
                      libusb_device *usb_device)
      : AsyncUsbSender(adaptor, usb_device) {
    m_control_setup_buffer =
      new uint8_t[LIBUSB_CONTROL_SETUP_SIZE + DMX_UNIVERSE_SIZE];
  }

  ~AnymaAsyncUsbSender() {
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
    libusb_fill_control_setup(
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
    return SubmitTransfer() == 0;
  }

 private:
  uint8_t *m_control_setup_buffer;

  DISALLOW_COPY_AND_ASSIGN(AnymaAsyncUsbSender);
};

// AsynchronousAnymaWidget
// -----------------------------------------------------------------------------

AsynchronousAnymaWidget::AsynchronousAnymaWidget(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const string &serial)
    : AnymaWidget(adaptor, serial) {
  m_sender.reset(new AnymaAsyncUsbSender(m_adaptor, usb_device));
}

bool AsynchronousAnymaWidget::Init() {
  return m_sender->Init();
}

bool AsynchronousAnymaWidget::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
