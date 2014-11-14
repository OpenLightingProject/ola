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
 * SunliteWidget.cpp
 * The synchronous and asynchronous Sunlite widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/SunliteWidget.h"

#include "ola/Constants.h"
#include "ola/Logging.h"
#include "plugins/usbdmx/LibUsbHelper.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

namespace {

static const unsigned int CHUNKS_PER_PACKET = 26;
static const unsigned int CHANNELS_PER_CHUNK = 20;
static const unsigned int CHUNK_SIZE = 32;
static const uint8_t ENDPOINT = 1;
static const unsigned int TIMEOUT = 50;  // 50ms is ok

/*
 * Called by the AsynchronousSunliteWidget when the transfer completes.
 */
void AsyncCallback(struct libusb_transfer *transfer) {
  AsynchronousSunliteWidget *widget =
      reinterpret_cast<AsynchronousSunliteWidget*>(transfer->user_data);
  widget->TransferComplete(transfer);
}

/*
 * Initialize a USBDMX2 packet
 */
void InitPacket(uint8_t packet[SunliteWidget::SUNLITE_PACKET_SIZE]) {
  memset(packet, 0, SunliteWidget::SUNLITE_PACKET_SIZE);

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
                  uint8_t packet[SunliteWidget::SUNLITE_PACKET_SIZE]) {
  for (unsigned int i = 0; i < buffer.Size(); i++) {
    packet[(i / CHANNELS_PER_CHUNK) * CHUNK_SIZE +
             ((i / 4) % 5) * 6 + 3 + (i % 4)] = buffer.Get(i);
  }
}

}  // namespace

/*
 * Sends messages to a Sunlite device in a separate thread.
 */
class SunliteThreadedSender: public ThreadedUsbSender {
 public:
  SunliteThreadedSender(libusb_device *usb_device,
                        libusb_device_handle *handle);

 private:
  uint8_t m_packet[SunliteWidget::SUNLITE_PACKET_SIZE];

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
};

SunliteThreadedSender::SunliteThreadedSender(
    libusb_device *usb_device,
    libusb_device_handle *usb_handle)
    : ThreadedUsbSender(usb_device, usb_handle) {
  InitPacket(m_packet);
}

bool SunliteThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                           const DmxBuffer &buffer) {
  UpdatePacket(buffer, m_packet);
  int transferred;
  int r = libusb_bulk_transfer(
      handle,
      ENDPOINT,
      (unsigned char*) m_packet,
      SunliteWidget::SUNLITE_PACKET_SIZE,
      &transferred,
      TIMEOUT);
  if (transferred != SunliteWidget::SUNLITE_PACKET_SIZE) {
    // not sure if this is fatal or not
    OLA_WARN << "Sunlite driver failed to transfer all data";
  }
  return r == 0;
}


SynchronousSunliteWidget::SynchronousSunliteWidget(libusb_device *usb_device)
    : m_usb_device(usb_device) {
}

bool SynchronousSunliteWidget::Init() {
  libusb_device_handle *usb_handle;

  bool ok = LibUsbHelper::OpenDeviceAndClaimInterface(
      m_usb_device, 0, &usb_handle);
  if (!ok) {
    return false;
  }

  std::auto_ptr<SunliteThreadedSender> sender(
      new SunliteThreadedSender(m_usb_device, usb_handle));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousSunliteWidget::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

AsynchronousSunliteWidget::AsynchronousSunliteWidget(
    libusb_device *usb_device)
    : m_usb_device(usb_device),
      m_usb_handle(NULL),
      m_transfer_state(IDLE) {
  InitPacket(m_packet);
  m_transfer = libusb_alloc_transfer(0);
  libusb_ref_device(usb_device);
}

AsynchronousSunliteWidget::~AsynchronousSunliteWidget() {
  bool canceled = false;
  OLA_INFO << "AsynchronousSunliteWidget shutdown";
  while (1) {
    ola::thread::MutexLocker locker(&m_mutex);
    if (m_transfer_state == IDLE) {
      break;
    }
    if (!canceled) {
      libusb_cancel_transfer(m_transfer);
      canceled = true;
    }
  }

  libusb_free_transfer(m_transfer);
  libusb_unref_device(m_usb_device);
}

bool AsynchronousSunliteWidget::Init() {
  bool ok = LibUsbHelper::OpenDeviceAndClaimInterface(
      m_usb_device, 0, &m_usb_handle);
  if (!ok) {
    return false;
  }
  return true;
}

bool AsynchronousSunliteWidget::SendDMX(const DmxBuffer &buffer) {
  OLA_INFO << "Call to AsynchronousSunliteWidget::SendDMX";
  if (!m_usb_handle) {
    OLA_WARN << "AsynchronousSunliteWidget hasn't been initialized";
    return false;
  }

  ola::thread::MutexLocker locker(&m_mutex);
  if (m_transfer_state != IDLE) {
    return true;
  }

  UpdatePacket(buffer, m_packet);
  libusb_fill_bulk_transfer(
      m_transfer,
      m_usb_handle,
      ENDPOINT,
      (unsigned char*) m_packet,
      SUNLITE_PACKET_SIZE,
      &AsyncCallback,
      this,
      TIMEOUT);

  int ret = libusb_submit_transfer(m_transfer);
  if (ret) {
    OLA_WARN << "libusb_submit_transfer returned " << libusb_error_name(ret);
    return false;
  }
  OLA_INFO << "submit ok";
  m_transfer_state = IN_PROGRESS;
  return true;
}

void AsynchronousSunliteWidget::TransferComplete(
    struct libusb_transfer *transfer) {
  if (transfer != m_transfer) {
    OLA_WARN << "Mismatched libusb transfer: " << transfer << " != "
             << m_transfer;
    return;
  }

  OLA_INFO << "async transfer complete";
  ola::thread::MutexLocker locker(&m_mutex);
  m_transfer_state = IDLE;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
