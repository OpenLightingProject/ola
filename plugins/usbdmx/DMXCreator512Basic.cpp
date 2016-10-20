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
 * DMXCreator512Basic.cpp
 * The synchronous and asynchronous DMXCreator512Basic widgets.
 * Copyright (C) 2016 Florian Edelmann
 */

#include "plugins/usbdmx/DMXCreator512Basic.h"

#include <unistd.h>
#include <string.h>
#include <string>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/Constants.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

/**
 * DMXCreator 512 Basic expects two or three URB packets for each change:
 * 1. A constant byte string to endpoint 1 that indicates if we want to transmit
 *    the the full universe or only the first half.
 * 2. The actual DMX data (channels 1..256) to endpoint 2.
 * 3. The actual DMX data (channels 257..512) to endpoint 2. (optional)
 */

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;
using ola::usb::LibUsbAdaptor;

namespace {

static const unsigned char ENDPOINT_1 = 0x01;
static const unsigned char ENDPOINT_2 = 0x02;
static const unsigned int URB_TIMEOUT_MS_SYNC = 1000;
static const unsigned int URB_TIMEOUT_MS_ASYNC = 50;
static const unsigned int CHANNELS_PER_PACKET = 256;

// if we only wanted to send the first half of the universe, the last byte would
// be 0x01
static const unsigned char status_buffer[6] = {
  0x80, 0x01, 0x00, 0x00, 0x00, 0x02
};

typedef enum {
  STATE_SEND_STATUS,
  STATE_SEND_FIRST_HALF,
  STATE_SEND_SECOND_HALF
} SendState;

}  // namespace

// DMXCreator512BasicThreadedSender
// -----------------------------------------------------------------------------

/**
 * Sends messages to a DMXCreator 512 Basic device in a separate thread.
 */
class DMXCreator512BasicThreadedSender: public ThreadedUsbSender {
 public:
  DMXCreator512BasicThreadedSender(LibUsbAdaptor *adaptor,
                           libusb_device *usb_device,
                           libusb_device_handle *handle)
      : ThreadedUsbSender(usb_device, handle),
        m_adaptor(adaptor) {
    m_dmx_buffer_1 = new uint8_t[CHANNELS_PER_PACKET];
    m_dmx_buffer_2 = new uint8_t[CHANNELS_PER_PACKET];
  }

 private:
  uint8_t *m_dmx_buffer_1;
  uint8_t *m_dmx_buffer_2;
  LibUsbAdaptor* const m_adaptor;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

bool DMXCreator512BasicThreadedSender::TransmitBuffer(
      libusb_device_handle *handle, const DmxBuffer &buffer) {
  unsigned int length = CHANNELS_PER_PACKET;
  memset(m_dmx_buffer_1, 0, length);
  buffer.Get(m_dmx_buffer_1, &length);

  length = CHANNELS_PER_PACKET;
  memset(m_dmx_buffer_2, 0, length);
  buffer.GetRange(CHANNELS_PER_PACKET, m_dmx_buffer_2, &length);

  int bytes_sent = 0;
  int r = m_adaptor->BulkTransfer(handle, ENDPOINT_1,
                                  const_cast<unsigned char*>(status_buffer),
                                  sizeof(status_buffer), &bytes_sent,
                                  URB_TIMEOUT_MS_SYNC);
  OLA_DEBUG << "Sending status bytes returned " << r;

  // Sometimes we get PIPE errors, those are non-fatal
  if (r < 0 && r != LIBUSB_ERROR_PIPE) {
    OLA_WARN << "Sending status bytes failed";
    return false;
  }

  bytes_sent = 0;
  r = m_adaptor->BulkTransfer(handle, ENDPOINT_2,
                              const_cast<unsigned char*>(m_dmx_buffer_1),
                              CHANNELS_PER_PACKET, &bytes_sent,
                              URB_TIMEOUT_MS_SYNC);
  OLA_DEBUG << "Sending data bytes (1) returned " << r;

  if (r < 0 && r != LIBUSB_ERROR_PIPE) {
    OLA_WARN << "Sending status bytes failed";
    return false;
  }

  bytes_sent = 0;
  r = m_adaptor->BulkTransfer(handle, ENDPOINT_2,
                              const_cast<unsigned char*>(m_dmx_buffer_2),
                              CHANNELS_PER_PACKET, &bytes_sent,
                              URB_TIMEOUT_MS_SYNC);
  OLA_DEBUG << "Sending data bytes (2) returned " << r;

  return r >= 0 || r == LIBUSB_ERROR_PIPE;
}


// SynchronousDMXCreator512Basic
// -----------------------------------------------------------------------------

SynchronousDMXCreator512Basic::SynchronousDMXCreator512Basic(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const std::string &serial)
    : DMXCreator512Basic(adaptor, usb_device, serial) {
}

bool SynchronousDMXCreator512Basic::Init() {
  libusb_device_handle *usb_handle;

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, 0, &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<DMXCreator512BasicThreadedSender> sender(
      new DMXCreator512BasicThreadedSender(m_adaptor, m_usb_device,
                                           usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousDMXCreator512Basic::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// DMXCreator512BasicAsyncUsbSender
// -----------------------------------------------------------------------------
class DMXCreator512BasicAsyncUsbSender : public AsyncUsbSender {
 public:
  DMXCreator512BasicAsyncUsbSender(LibUsbAdaptor *adaptor,
                                   libusb_device *usb_device)
      : AsyncUsbSender(adaptor, usb_device),
        m_adaptor(adaptor),
        m_usb_device(usb_device) {
    m_dmx_buffer_1 = new uint8_t[CHANNELS_PER_PACKET];
    m_dmx_buffer_2 = new uint8_t[CHANNELS_PER_PACKET];
    m_state = STATE_SEND_STATUS;
  }

  ~DMXCreator512BasicAsyncUsbSender() {
    CancelTransfer();
    delete[] m_dmx_buffer_1;
    delete[] m_dmx_buffer_2;
  }

  libusb_device_handle* SetupHandle() {
    libusb_device_handle *usb_handle;
    bool ok = m_adaptor->OpenDeviceAndClaimInterface(
        m_usb_device, 0, &usb_handle);
    return ok ? usb_handle : NULL;
  }

  bool PerformTransfer(const DmxBuffer &buffer) {
    unsigned int length = CHANNELS_PER_PACKET;
    memset(m_dmx_buffer_1, 0, length);
    buffer.Get(m_dmx_buffer_1, &length);

    length = CHANNELS_PER_PACKET;
    memset(m_dmx_buffer_2, 0, length);
    buffer.GetRange(CHANNELS_PER_PACKET, m_dmx_buffer_2, &length);

    m_state = STATE_SEND_FIRST_HALF;
    FillBulkTransfer(ENDPOINT_1,
                     const_cast<unsigned char*>(status_buffer),
                     sizeof(status_buffer), URB_TIMEOUT_MS_ASYNC);
    return (SubmitTransfer() == 0);
  }

  void PostTransferHook() {
    switch (m_state) {
      case STATE_SEND_STATUS:
        // handled in PerformTransfer()
        break;
      case STATE_SEND_FIRST_HALF:
        m_state = STATE_SEND_SECOND_HALF;
        FillBulkTransfer(ENDPOINT_2,
                         const_cast<unsigned char*>(m_dmx_buffer_1),
                         CHANNELS_PER_PACKET, URB_TIMEOUT_MS_ASYNC);
        SubmitTransfer();
        break;
      case STATE_SEND_SECOND_HALF:
        m_state = STATE_SEND_STATUS;
        FillBulkTransfer(ENDPOINT_2,
                         const_cast<unsigned char*>(m_dmx_buffer_2),
                         CHANNELS_PER_PACKET, URB_TIMEOUT_MS_ASYNC);
        SubmitTransfer();
        break;
    }
  }


 private:
  uint8_t *m_dmx_buffer_1;
  uint8_t *m_dmx_buffer_2;
  SendState m_state;
  ola::usb::LibUsbAdaptor* const m_adaptor;
  libusb_device* const m_usb_device;

  DISALLOW_COPY_AND_ASSIGN(DMXCreator512BasicAsyncUsbSender);
};

// AsynchronousDMXCreator512Basic
// -----------------------------------------------------------------------------

AsynchronousDMXCreator512Basic::AsynchronousDMXCreator512Basic(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    const string &serial)
    : DMXCreator512Basic(adaptor, usb_device, serial) {
  m_sender.reset(new DMXCreator512BasicAsyncUsbSender(m_adaptor, usb_device));
}

bool AsynchronousDMXCreator512Basic::Init() {
  return m_sender->Init();
}

bool AsynchronousDMXCreator512Basic::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}

}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
