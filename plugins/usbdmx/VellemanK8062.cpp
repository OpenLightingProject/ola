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
 * VellemanK8062.cpp
 * The synchronous and asynchronous Velleman widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/VellemanK8062.h"

#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <string>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/Logging.h"
#include "ola/Constants.h"
#include "ola/StringUtils.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using ola::usb::LibUsbAdaptor;
using std::string;

namespace {

static const unsigned char ENDPOINT = 0x01;
// 25ms seems to be about the shortest we can go
static const unsigned int URB_TIMEOUT_MS = 25;
static const int CONFIGURATION = 1;
static const int INTERFACE = 0;
static const unsigned int DEFAULT_CHUNK_SIZE = 8;
static const unsigned int UPGRADED_CHUNK_SIZE = 64;
static const unsigned int HEADER_SIZE = 2;

// Message types
// Length: 8 or 64 for the extended version.
// Data: [2] [slot N] [slot N + 1] [slot N + 2] ... [slot N + 6]
static const uint8_t INTERMEDIATE_FRAME_MSG = 2;

// Length: 8 or 64 for the extended version.
// Data: [3] [slot N] [undef] [undef] [undef] ...
static const uint8_t SINGLE_SLOT_MSG = 3;

// This must be used to indicate a new DMX512 frame.
// Length: 8 or 64 for the extended version.
// Data: [4] [number of leading 0s] [slot N] [slot N + 1] [slot N + 2] ...
static const uint8_t BREAK_MSG = 4;

// Length: 8 or 64 for the extended version.
// [5] [number of leading 0s] [slot N] [slot N + 1] [slot N + 2] ...
static const uint8_t INTERMEDIATE_COMPRESSED_FRAME_MSG = 5;

// Length: 64, only defined for the extended version.
// Data: [4] [data size] [slot 0] [slot 1] [slot 2] ...
static const uint8_t VARIABLE_FRAME_CONTINUATION_MSG = 6;

// Length: 64, only defined for the extended version.
// Data: [4] [data size] [slot 0] [slot 1] [slot 2] ...
static const uint8_t FULL_FRAME_MSG = 7;

/*
 * @brief Attempt to open a handle to a Velleman widget.
 * @param adaptor the LibUsbAdaptor to use.
 * @param usb_device the libusb_device to use.
 * @param[out] The chunk size of the device, this determines if the enhanced
 *   firmware is present.
 * @returns A libusb_device_handle of NULL if it failed.
 */
libusb_device_handle *OpenVellemenWidget(LibUsbAdaptor *adaptor,
                                         libusb_device *usb_device,
                                         unsigned int *chunk_size) {
  libusb_config_descriptor *config;
  if (adaptor->GetActiveConfigDescriptor(usb_device, &config)) {
    OLA_WARN << "Could not get active config descriptor";
    return NULL;
  }

  // determine the max packet size, see
  // https://wiki.openlighting.org/index.php/Velleman_K8062_Upgrade
  // The standard size is 8.
  *chunk_size = DEFAULT_CHUNK_SIZE;
  if (config &&
      config->interface &&
      config->interface->altsetting &&
      config->interface->altsetting->endpoint) {
    uint16_t max_packet_size =
      config->interface->altsetting->endpoint->wMaxPacketSize;
    OLA_DEBUG << "Velleman K8062 max packet size is " << max_packet_size;
    if (max_packet_size == UPGRADED_CHUNK_SIZE) {
      // this means the upgrade is present
      *chunk_size = max_packet_size;
    }
  }
  adaptor->FreeConfigDescriptor(config);

  libusb_device_handle *usb_handle;
  bool ok = adaptor->OpenDevice(usb_device, &usb_handle);
  if (!ok) {
    return NULL;
  }

  int ret_code = adaptor->DetachKernelDriver(usb_handle, INTERFACE);
  if (ret_code != 0 && ret_code != LIBUSB_ERROR_NOT_FOUND) {
    OLA_WARN << "Failed to detach kernel driver: "
             << adaptor->ErrorCodeToString(ret_code);
    adaptor->Close(usb_handle);
    return NULL;
  }

  // this device only has one configuration
  ret_code = adaptor->SetConfiguration(usb_handle, CONFIGURATION);
  if (ret_code) {
    OLA_WARN << "Velleman set config failed, with libusb error code "
             << ret_code;
    adaptor->Close(usb_handle);
    return NULL;
  }

  if (adaptor->ClaimInterface(usb_handle, INTERFACE)) {
    OLA_WARN << "Failed to claim Velleman usb device";
    adaptor->Close(usb_handle);
    return NULL;
  }
  return usb_handle;
}

/*
 * @brief Count the number of leading 0s in a block of data.
 * @params data the data to count the 0s in.
 * @param data_length the length of the data.
 * @param chunk_size the size of the chunks we send to the widget.
 * @returns the number of leading 0s in the data.
 */
unsigned int CountLeadingZeros(const uint8_t *data, unsigned int data_length,
                               unsigned int chunk_size) {
  unsigned int leading_zero_count = 0;

  // This could be up to 254 for the standard interface but then the shutdown
  // process gets wacky. Limit it to 100 for the standard and 255 for the
  // extended.
  unsigned int max_leading_zeros = (chunk_size == UPGRADED_CHUNK_SIZE) ?
    254 : 100;
  unsigned int rest_of_chunk = chunk_size - 2;

  while (leading_zero_count < max_leading_zeros &&
         leading_zero_count + rest_of_chunk < data_length &&
         data[leading_zero_count] == 0) {
    leading_zero_count++;
  }
  return leading_zero_count;
}

}  // namespace

// VellemanThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a Velleman device in a separate thread.
 */
class VellemanThreadedSender: public ThreadedUsbSender {
 public:
  VellemanThreadedSender(LibUsbAdaptor *adaptor,
                         libusb_device *usb_device,
                         libusb_device_handle *handle,
                         unsigned int chunk_size)
      : ThreadedUsbSender(usb_device, handle),
        m_adaptor(adaptor),
        m_chunk_size(chunk_size) {
    m_tx_buffer.Blackout();
  }

 private:
  LibUsbAdaptor* const m_adaptor;
  DmxBuffer m_tx_buffer;
  const unsigned int m_chunk_size;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
  bool SendDataChunk(libusb_device_handle *handle,
                     uint8_t *usb_data,
                     unsigned int chunk_size);
};

bool VellemanThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                            const DmxBuffer &buffer) {
  m_tx_buffer.SetRange(0, buffer.GetRaw(), buffer.Size());

  unsigned char usb_data[m_chunk_size];
  const unsigned int size = m_tx_buffer.Size();
  const uint8_t *data = m_tx_buffer.GetRaw();
  unsigned int i = 0;

  unsigned int compressed_channel_count = m_chunk_size - 2;
  unsigned int channel_count = m_chunk_size - 1;

  memset(usb_data, 0, sizeof(usb_data));

  if (m_chunk_size == UPGRADED_CHUNK_SIZE && size <= m_chunk_size - 2) {
    // if the upgrade is present and we can fit the data in a single packet
    // use FULL_FRAME_MSG.
    usb_data[0] = FULL_FRAME_MSG;
    usb_data[1] = size;  // number of channels in packet
    memcpy(usb_data + HEADER_SIZE, data, std::min(size, m_chunk_size - 2));
  } else {
    unsigned int leading_zero_count = CountLeadingZeros(
        data, size, m_chunk_size);
    usb_data[0] = BREAK_MSG;
    usb_data[1] = leading_zero_count + 1;  // include start code
    memcpy(usb_data + HEADER_SIZE, data + leading_zero_count,
           compressed_channel_count);
    i += leading_zero_count + compressed_channel_count;
  }

  if (!SendDataChunk(handle, usb_data, m_chunk_size)) {
    return false;
  }

  while (i < size - channel_count) {
    unsigned int leading_zero_count = CountLeadingZeros(
        data + i, size - i, m_chunk_size);
    if (leading_zero_count) {
      // we have leading zeros
      usb_data[0] = INTERMEDIATE_COMPRESSED_FRAME_MSG;
      usb_data[1] = leading_zero_count;
      memcpy(usb_data + HEADER_SIZE, data + i + leading_zero_count,
             compressed_channel_count);
      i += leading_zero_count + compressed_channel_count;
    } else {
      usb_data[0] = INTERMEDIATE_FRAME_MSG;
      memcpy(usb_data + 1, data + i, channel_count);
      i += channel_count;
    }
    if (!SendDataChunk(handle, usb_data, m_chunk_size)) {
      return false;
    }
  }

  // send the last channels
  if (m_chunk_size == UPGRADED_CHUNK_SIZE) {
    // if running in extended mode we can use the 6 message type to send
    // everything at once.
    usb_data[0] = VARIABLE_FRAME_CONTINUATION_MSG;
    usb_data[1] = size - i;
    memcpy(usb_data + HEADER_SIZE, data + i, size - i);
    if (!SendDataChunk(handle, usb_data, m_chunk_size)) {
      return false;
    }

  } else {
    // else we use the 3 message type to send one at a time
    for (; i != size; i++) {
      usb_data[0] = SINGLE_SLOT_MSG;
      usb_data[1] = data[i];
      if (!SendDataChunk(handle, usb_data, m_chunk_size)) {
        return false;
      }
    }
  }
  return true;
}

bool VellemanThreadedSender::SendDataChunk(libusb_device_handle *handle,
                                           uint8_t *usb_data,
                                           unsigned int chunk_size) {
  int transferred;
  int ret = m_adaptor->InterruptTransfer(
      handle, ENDPOINT, reinterpret_cast<unsigned char*>(usb_data),
      chunk_size, &transferred, URB_TIMEOUT_MS);
  if (ret) {
    OLA_WARN << "InterruptTransfer(): " << m_adaptor->ErrorCodeToString(ret)
             << ", transferred " << transferred << " / " << chunk_size;
  }
  return ret == 0;
}

// SynchronousVellemanK8062
// -----------------------------------------------------------------------------

SynchronousVellemanK8062::SynchronousVellemanK8062(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device)
    : VellemanK8062(adaptor, usb_device) {
}

bool SynchronousVellemanK8062::Init() {
  unsigned int chunk_size = DEFAULT_CHUNK_SIZE;
  libusb_device_handle *usb_handle = OpenVellemenWidget(
      m_adaptor, m_usb_device, &chunk_size);

  if (!usb_handle) {
    return false;
  }

  std::auto_ptr<VellemanThreadedSender> sender(
      new VellemanThreadedSender(m_adaptor, m_usb_device, usb_handle,
                                 chunk_size));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousVellemanK8062::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

// VellemanAsyncUsbSender
// -----------------------------------------------------------------------------
class VellemanAsyncUsbSender : public AsyncUsbSender {
 public:
  VellemanAsyncUsbSender(LibUsbAdaptor *adaptor,
                         libusb_device *usb_device)
      : AsyncUsbSender(adaptor, usb_device),
        m_chunk_size(DEFAULT_CHUNK_SIZE),
        m_buffer_offset(0),
        m_packet(NULL) {
    m_tx_buffer.Blackout();
  }

  ~VellemanAsyncUsbSender() {
    CancelTransfer();
    if (m_packet) {
      delete[] m_packet;
    }
  }

  libusb_device_handle* SetupHandle() {
    libusb_device_handle *handle = OpenVellemenWidget(m_adaptor, m_usb_device,
                                                      &m_chunk_size);
    m_packet = new uint8_t[m_chunk_size];
    return handle;
  }

  bool PerformTransfer(const DmxBuffer &buffer);

  void PostTransferHook();

 private:
  // These are set once we known the type of device we're talking to.
  unsigned int m_chunk_size;

  DmxBuffer m_tx_buffer;
  // This tracks where were are in m_tx_buffer. A value of 0 means we're at the
  // start of a DMX frame.
  unsigned int m_buffer_offset;
  uint8_t *m_packet;

  bool ContinueTransfer();

  bool SendInitialChunk(const DmxBuffer &buffer);
  bool SendIntermediateChunk();
  bool SendSingleSlotChunk();

  bool SendChunk() {
    FillInterruptTransfer(ENDPOINT, m_packet, m_chunk_size, URB_TIMEOUT_MS);
    return (SubmitTransfer() == 0);
  }

  DISALLOW_COPY_AND_ASSIGN(VellemanAsyncUsbSender);
};

bool VellemanAsyncUsbSender::PerformTransfer(const DmxBuffer &buffer) {
  if (m_buffer_offset == 0) {
    return SendInitialChunk(buffer);
  }
  // Otherwise we're part way through a transfer, do nothing.
  return true;
}

void VellemanAsyncUsbSender::PostTransferHook() {
  if (m_buffer_offset < m_tx_buffer.Size()) {
    ContinueTransfer();
  } else if (m_buffer_offset >= m_tx_buffer.Size()) {
    // That was the last chunk.
    m_buffer_offset = 0;

    if (TransferPending()) {
      // If we have a pending transfer, the next chunk is going to be sent
      // once we return.
      m_tx_buffer.Reset();
    } else {
      // No pending transfer. The widget only actually sends a frame once the
      // next frame begins, so kick off the next frame here.
      PerformTransfer(m_tx_buffer);
    }
  }
}

bool VellemanAsyncUsbSender::ContinueTransfer() {
  if (m_buffer_offset + m_chunk_size < m_tx_buffer.Size()) {
    return SendIntermediateChunk();
  }

  if (m_chunk_size == UPGRADED_CHUNK_SIZE) {
    // If running in extended mode we can use the 6 message type to send
    // everything at once.
    unsigned int length = m_chunk_size - HEADER_SIZE;

    m_packet[0] = VARIABLE_FRAME_CONTINUATION_MSG;
    m_packet[1] = m_tx_buffer.Size() - m_buffer_offset;
    m_tx_buffer.GetRange(m_buffer_offset, m_packet + HEADER_SIZE, &length);
    memset(m_packet + HEADER_SIZE + length, 0,
           m_chunk_size - length - HEADER_SIZE);
    return (SendChunk() == 0);
  } else {
    // The trailing slots are sent individually.
    return SendSingleSlotChunk();
  }
}

bool VellemanAsyncUsbSender::SendInitialChunk(const DmxBuffer &buffer) {
  unsigned int length = m_chunk_size - HEADER_SIZE;

  if (m_chunk_size == UPGRADED_CHUNK_SIZE &&
      buffer.Size() <= m_chunk_size - HEADER_SIZE) {
    // If the upgrade is present and we can fit the data in a single chunk
    // use the FULL_FRAME_MSG message type.
    m_packet[0] = FULL_FRAME_MSG;
    m_packet[1] = m_tx_buffer.Size();  // number of slots in the frame.
    buffer.Get(m_packet + HEADER_SIZE, &length);
    memset(m_packet + HEADER_SIZE + length, 0,
           m_chunk_size - length - HEADER_SIZE);
  } else {
    m_tx_buffer.SetRange(0, buffer.GetRaw(), buffer.Size());

    // Otherwise use BREAK_MSG to signal the start of frame.
    unsigned int leading_zero_count = CountLeadingZeros(
        m_tx_buffer.GetRaw(), m_tx_buffer.Size(), m_chunk_size);
    m_packet[0] = BREAK_MSG;
    m_packet[1] = leading_zero_count + 1;  // include start code
    m_tx_buffer.GetRange(leading_zero_count, m_packet + HEADER_SIZE, &length);
    memset(m_packet + HEADER_SIZE + length, 0,
           m_chunk_size - length - HEADER_SIZE);

    unsigned int slots_sent = leading_zero_count + length;
    if (slots_sent < m_tx_buffer.Size()) {
      // There are more frames to send.
      m_buffer_offset = slots_sent;
    }
  }
  return (SendChunk() == 0);
}

bool VellemanAsyncUsbSender::SendIntermediateChunk() {
  // Intermediate frame.
  unsigned int zeros = CountLeadingZeros(
      m_tx_buffer.GetRaw() + m_buffer_offset,
      m_tx_buffer.Size() - m_buffer_offset,
      m_chunk_size);

  unsigned int length = m_chunk_size - 1;
  if (zeros) {
    // we have leading zeros
    m_packet[0] = INTERMEDIATE_COMPRESSED_FRAME_MSG;
    m_packet[1] = zeros;
    length--;
    m_tx_buffer.GetRange(m_buffer_offset + zeros, m_packet + HEADER_SIZE,
                         &length);
    m_buffer_offset += zeros + length;
  } else {
    m_packet[0] = INTERMEDIATE_FRAME_MSG;
    m_tx_buffer.GetRange(m_buffer_offset, m_packet + 1, &length);
    memset(m_packet + 1 + length, 0, m_chunk_size - length - 1);
    m_buffer_offset += length;
  }
  return (SendChunk() == 0);
}

bool VellemanAsyncUsbSender::SendSingleSlotChunk() {
  memset(m_packet, 0, m_chunk_size);
  m_packet[0] = SINGLE_SLOT_MSG;
  m_packet[1] = m_tx_buffer.Get(m_buffer_offset);
  m_buffer_offset++;
  return (SendChunk() == 0);
}

// AsynchronousVellemanK8062
// -----------------------------------------------------------------------------

AsynchronousVellemanK8062::AsynchronousVellemanK8062(
    LibUsbAdaptor *adaptor,
    libusb_device *usb_device)
    : VellemanK8062(adaptor, usb_device) {
  m_sender.reset(new VellemanAsyncUsbSender(m_adaptor, usb_device));
}

bool AsynchronousVellemanK8062::Init() {
  return m_sender->Init();
}

bool AsynchronousVellemanK8062::SendDMX(const DmxBuffer &buffer) {
  return m_sender->SendDMX(buffer);
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
