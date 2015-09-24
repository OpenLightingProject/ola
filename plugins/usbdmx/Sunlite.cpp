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
 * Sunlite.cpp
 * The synchronous and asynchronous Sunlite widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/Sunlite.h"

#include <string.h>

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

static const unsigned int CHUNKS_PER_PACKET = 26;
static const unsigned int CHANNELS_PER_CHUNK = 20;
static const unsigned int CHUNK_SIZE = 32;
static const uint8_t ENDPOINT = 1;
static const unsigned int TIMEOUT = 50;  // 50ms is ok
enum {SUNLITE_PACKET_SIZE = 0x340};

/*
 * Initialize a USBDMX2 packet
 */
void InitPacket(uint8_t packet[SUNLITE_PACKET_SIZE]) {
  memset(packet, 0, SUNLITE_PACKET_SIZE);

  // The packet is divided into 26 chunks of 32 bytes each. Each chunk contains
  // the data for 20 channels (except the last one which has 12 channels of
  // data).
  for (unsigned int chunk = 0; chunk < CHUNKS_PER_PACKET; ++chunk) {
    unsigned int i = chunk * CHUNK_SIZE;  // index into the packet
    unsigned int channel = chunk * CHANNELS_PER_CHUNK;

    packet[i] = 0x80;
    packet[i + 1] = channel / 2;
    packet[i + 2] = 0x84;
    packet[i + 7] = channel / 2 + 2;
    packet[i + 8] = 0x84;
    packet[i + 13] = channel / 2 + 4;
    if (chunk < CHUNKS_PER_PACKET - 1) {
      packet[i + 14] = 0x84;
      packet[i + 19] = channel / 2 + 6;
      packet[i + 20] = 0x84;
      packet[i + 25] = channel / 2 + 8;
      packet[i + 26] = 0x04;
      packet[i + 31] = 0x00;
    } else {
      // the last chunk is short
      packet[i + 14] = 0x04;
    }
  }
}

/*
 * Update a USBDMX2 packet to match the supplied DmxBuffer.
 */
void UpdatePacket(const DmxBuffer &buffer,
                  uint8_t packet[SUNLITE_PACKET_SIZE]) {
  for (unsigned int i = 0; i < buffer.Size(); i++) {
    int index = ((i / CHANNELS_PER_CHUNK) * CHUNK_SIZE) +
                (((i / 4) % 5) * 6) + 3 + (i % 4);
    packet[index] = buffer.Get(i);
  }
}

}  // namespace

// SunliteThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a Sunlite device in a separate thread.
 */
class SunliteThreadedSender: public ThreadedUsbSender {
 public:
  SunliteThreadedSender(LibUsbAdaptor *adaptor,
                        libusb_device *usb_device,
                        libusb_device_handle *handle);

 private:
  LibUsbAdaptor* const m_adaptor;
  uint8_t m_packet[SUNLITE_PACKET_SIZE];

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

SunliteThreadedSender::SunliteThreadedSender(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    libusb_device_handle *usb_handle)
    : ThreadedUsbSender(usb_device, usb_handle),
      m_adaptor(adaptor) {
  InitPacket(m_packet);
}

bool SunliteThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                           const DmxBuffer &buffer) {
  UpdatePacket(buffer, m_packet);
  int transferred;
  int r = m_adaptor->BulkTransfer(handle, ENDPOINT, (unsigned char*) m_packet,
                                  SUNLITE_PACKET_SIZE, &transferred, TIMEOUT);
  if (transferred != SUNLITE_PACKET_SIZE) {
    // not sure if this is fatal or not
    OLA_WARN << "Sunlite driver failed to transfer all data";
  }
  return r == 0;
}

// SynchronousSunlite
// -----------------------------------------------------------------------------

SynchronousSunlite::SynchronousSunlite(LibUsbAdaptor *adaptor,
                                                   libusb_device *usb_device)
    : Sunlite(adaptor, usb_device) {
}

bool SynchronousSunlite::Init() {
  libusb_device_handle *usb_handle;

  bool ok = m_adaptor->OpenDeviceAndClaimInterface(
      m_usb_device, 0, &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<SunliteThreadedSender> sender(
      new SunliteThreadedSender(m_adaptor, m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousSunlite::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// SunliteAsyncUsbSender
// -----------------------------------------------------------------------------
class SunliteAsyncUsbSender : public AsyncUsbSender {
 public:
  SunliteAsyncUsbSender(LibUsbAdaptor *adaptor,
                        libusb_device *usb_device)
      : AsyncUsbSender(adaptor, usb_device) {
    InitPacket(m_packet);
  }

  ~SunliteAsyncUsbSender() {
    CancelTransfer();
  }

  libusb_device_handle* SetupHandle() {
    libusb_device_handle *usb_handle;
    bool ok = m_adaptor->OpenDeviceAndClaimInterface(
        m_usb_device, 0, &usb_handle);
    return ok ? usb_handle : NULL;
  }

  bool PerformTransfer(const DmxBuffer &buffer) {
    UpdatePacket(buffer, m_packet);
    FillBulkTransfer(ENDPOINT, m_packet, SUNLITE_PACKET_SIZE, TIMEOUT);
    return (SubmitTransfer() == 0);
  }

 private:
  uint8_t m_packet[SUNLITE_PACKET_SIZE];

  DISALLOW_COPY_AND_ASSIGN(SunliteAsyncUsbSender);
};

// AsynchronousSunlite
// -----------------------------------------------------------------------------

AsynchronousSunlite::AsynchronousSunlite(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device)
    : Sunlite(adaptor, usb_device) {
  m_sender.reset(new SunliteAsyncUsbSender(m_adaptor, usb_device));
}

bool AsynchronousSunlite::Init() {
  return m_sender->Init();
}

bool AsynchronousSunlite::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
