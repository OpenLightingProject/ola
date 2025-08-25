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
#include <utility>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/Constants.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

/**
 * DMXCreator 512 Basic expects two or three URB packets for each change:
 * 1. A constant byte string to endpoint 1 that indicates if we want to transmit
 *    the full universe or only the first half.
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
static const unsigned int URB_TIMEOUT_MS = 50;
static const unsigned int CHANNELS_PER_PACKET = 256;

// if we only wanted to send the first half of the universe, the last byte would
// be 0x01
static const uint8_t status_buffer[6] = {
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
        m_adaptor(adaptor) {}

 private:
  DmxBuffer m_dmx_buffer;
  uint8_t m_universe_data_lower[CHANNELS_PER_PACKET];
  uint8_t m_universe_data_upper[CHANNELS_PER_PACKET];
  LibUsbAdaptor* const m_adaptor;

  bool TransmitBuffer(libusb_device_handle *handle, const DmxBuffer &buffer);

  bool BulkTransferPart(libusb_device_handle *handle, unsigned char endpoint,
                        const uint8_t *buffer, size_t size,
                        const std::string name);
};

bool DMXCreator512BasicThreadedSender::TransmitBuffer(
    libusb_device_handle *handle, const DmxBuffer &buffer) {

  if (m_dmx_buffer == buffer) {
    // no need to update -> sleep 50µs to avoid timeout errors
    usleep(50);
    return true;
  }

  m_dmx_buffer = buffer;

  unsigned int length = CHANNELS_PER_PACKET;
  m_dmx_buffer.Get(m_universe_data_lower, &length);
  memset(m_universe_data_lower + length, 0, CHANNELS_PER_PACKET - length);

  length = CHANNELS_PER_PACKET;
  m_dmx_buffer.GetRange(CHANNELS_PER_PACKET, m_universe_data_upper, &length);
  memset(m_universe_data_upper + length, 0, CHANNELS_PER_PACKET - length);

  bool r = BulkTransferPart(handle, ENDPOINT_1, status_buffer,
                            sizeof(status_buffer), "status");
  if (!r) {
    return false;
  }

  r = BulkTransferPart(handle, ENDPOINT_2, m_universe_data_lower,
                       CHANNELS_PER_PACKET, "lower data");
  if (!r) {
    return false;
  }

  r = BulkTransferPart(handle, ENDPOINT_2, m_universe_data_upper,
                       CHANNELS_PER_PACKET, "upper data");
  return r;
}

bool DMXCreator512BasicThreadedSender::BulkTransferPart(
    libusb_device_handle *handle, unsigned char endpoint,
    const uint8_t *buffer, size_t size, const std::string name) {
  int bytes_sent = 0;
  int r = m_adaptor->BulkTransfer(handle, endpoint,
                                  const_cast<unsigned char*>(buffer),
                                  size, &bytes_sent, URB_TIMEOUT_MS);

  // Sometimes we get PIPE errors, those are non-fatal
  if (r < 0 && r != LIBUSB_ERROR_PIPE) {
    OLA_WARN << "Sending " << name << " bytes failed: "
             << m_adaptor->ErrorCodeToString(r);
    return false;
  }
  return true;
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

  std::unique_ptr<DMXCreator512BasicThreadedSender> sender(
      new DMXCreator512BasicThreadedSender(m_adaptor, m_usb_device,
                                           usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender = std::move(sender);
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
    m_state = STATE_SEND_STATUS;
  }

  ~DMXCreator512BasicAsyncUsbSender() {
    CancelTransfer();
  }

  libusb_device_handle* SetupHandle() {
    libusb_device_handle *usb_handle;
    bool ok = m_adaptor->OpenDeviceAndClaimInterface(
        m_usb_device, 0, &usb_handle);
    return ok ? usb_handle : NULL;
  }

  bool PerformTransfer(const DmxBuffer &buffer) {
    unsigned int length = CHANNELS_PER_PACKET;
    buffer.Get(m_universe_data_lower, &length);
    memset(m_universe_data_lower + length, 0, CHANNELS_PER_PACKET - length);

    length = CHANNELS_PER_PACKET;
    buffer.GetRange(CHANNELS_PER_PACKET, m_universe_data_upper, &length);
    memset(m_universe_data_upper + length, 0, CHANNELS_PER_PACKET - length);

    m_state = STATE_SEND_FIRST_HALF;
    FillBulkTransfer(ENDPOINT_1,
                     const_cast<unsigned char*>(status_buffer),
                     sizeof(status_buffer), URB_TIMEOUT_MS);
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
                         const_cast<unsigned char*>(m_universe_data_lower),
                         CHANNELS_PER_PACKET, URB_TIMEOUT_MS);
        SubmitTransfer();
        break;
      case STATE_SEND_SECOND_HALF:
        m_state = STATE_SEND_STATUS;
        FillBulkTransfer(ENDPOINT_2,
                         const_cast<unsigned char*>(m_universe_data_upper),
                         CHANNELS_PER_PACKET, URB_TIMEOUT_MS);
        SubmitTransfer();
        break;
    }
  }


 private:
  uint8_t m_universe_data_lower[CHANNELS_PER_PACKET];
  uint8_t m_universe_data_upper[CHANNELS_PER_PACKET];
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
