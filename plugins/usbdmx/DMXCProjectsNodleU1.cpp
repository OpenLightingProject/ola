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
 * DMXCProjectsNodleU1.cpp
 * The synchronous and asynchronous Nodle widgets.
 * Copyright (C) 2015 Stefan Krupop
 */

#include "plugins/usbdmx/DMXCProjectsNodleU1.h"

#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <string>

#include "ola/Logging.h"
#include "ola/Constants.h"
#include "ola/StringUtils.h"
#include "plugins/usbdmx/AsyncUsbReceiver.h"
#include "plugins/usbdmx/AsyncUsbSender.h"
#include "plugins/usbdmx/ThreadedUsbReceiver.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

int DMXCProjectsNodleU1::NODLE_DEFAULT_MODE = 6;
int DMXCProjectsNodleU1::NODLE_MIN_MODE = 0;
int DMXCProjectsNodleU1::NODLE_MAX_MODE = 7;
int DMXCProjectsNodleU1::OUTPUT_ENABLE_MASK = 2;
int DMXCProjectsNodleU1::INPUT_ENABLE_MASK = 4;

namespace {

static const unsigned char WRITE_ENDPOINT = 0x02;
static const unsigned char READ_ENDPOINT = 0x81;
// On a non-overclocked Raspberry Pi 1 the previous value of 50ms lead to some
// timeout transfer errors. Changing to 60 and a nice level of -20 made them go
// away. Changing to 70 made them go away without touching the nice level.
static const unsigned int URB_TIMEOUT_MS = 70;
static const int CONFIGURATION = 1;
static const int INTERFACE = 0;

static const unsigned int DATABLOCK_SIZE = 33;

/*
 * @brief Send chosen mode to the DMX device
 * @param handle the libusb_device_handle to use.
 * @returns true if mode was set
 */
bool SetInterfaceMode(ola::usb::LibUsbAdaptor *adaptor,
                      libusb_device_handle *handle,
                      uint8_t mode) {
  unsigned char usb_data[DATABLOCK_SIZE];
  int transferred;

  memset(usb_data, 0, sizeof(usb_data));
  usb_data[0] = 16;
  usb_data[1] = mode;

  int ret = adaptor->InterruptTransfer(
      handle, WRITE_ENDPOINT, reinterpret_cast<unsigned char*>(usb_data),
      DATABLOCK_SIZE, &transferred, URB_TIMEOUT_MS);
  if (ret) {
    OLA_WARN << "InterruptTransfer(): " << adaptor->ErrorCodeToString(ret)
             << ", transferred " << transferred << " / " << DATABLOCK_SIZE;
  }
  return ret == 0;
}


/*
 * @brief Attempt to open a handle to a Nodle widget.
 * @param adaptor the LibUsbAdaptor to use.
 * @param usb_device the libusb_device to use.
 * @returns A libusb_device_handle or NULL if it failed.
 */
libusb_device_handle *OpenDMXCProjectsNodleU1Widget(
    ola::usb::LibUsbAdaptor *adaptor,
    libusb_device *usb_device) {
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
    OLA_WARN << "Nodle set config failed, with libusb error code "
             << adaptor->ErrorCodeToString(ret_code);
    adaptor->Close(usb_handle);
    return NULL;
  }

  if (adaptor->ClaimInterface(usb_handle, INTERFACE)) {
    OLA_WARN << "Failed to claim Nodle USB device";
    adaptor->Close(usb_handle);
    return NULL;
  }

  return usb_handle;
}
}  // namespace

// DMXCProjectsNodleU1ThreadedSender
// -----------------------------------------------------------------------------

/*
 * Sends messages to a Nodle device in a separate thread.
 */
class DMXCProjectsNodleU1ThreadedSender: public ThreadedUsbSender {
 public:
  DMXCProjectsNodleU1ThreadedSender(ola::usb::LibUsbAdaptor *adaptor,
                                    libusb_device *usb_device,
                                    libusb_device_handle *handle)
      : ThreadedUsbSender(usb_device, handle),
        m_adaptor(adaptor) {
    m_last_tx_buffer.Blackout();
  }

 private:
  ola::usb::LibUsbAdaptor* const m_adaptor;
  DmxBuffer m_last_tx_buffer;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
  bool SendDataChunk(libusb_device_handle *handle,
                     uint8_t *usb_data);
};

bool DMXCProjectsNodleU1ThreadedSender::TransmitBuffer(
    libusb_device_handle *handle,
    const DmxBuffer &buffer) {
  unsigned char usb_data[DATABLOCK_SIZE];
  const unsigned int size = buffer.Size();
  const uint8_t *data = buffer.GetRaw();
  const uint8_t *last_data = m_last_tx_buffer.GetRaw();
  unsigned int i = 0;

  memset(usb_data, 0, sizeof(usb_data));

  while (i < size - 32) {
    if (memcmp(data + i, last_data + i, 32) != 0) {
      usb_data[0] = i / 32;
      memcpy(usb_data + 1, data + i, 32);
      m_last_tx_buffer.SetRange(i, data + i, 32);
      if (!SendDataChunk(handle, usb_data)) {
        return false;
      }
    }
    i += 32;
  }

  if (memcmp(data + i, last_data + i, size - i) != 0) {
    usb_data[0] = i / 32;
    memcpy(usb_data + 1, data + i, size - i);
    m_last_tx_buffer.SetRange(i, data + i, size - i);
    if (!SendDataChunk(handle, usb_data)) {
      return false;
    }
  }

  return true;
}

bool DMXCProjectsNodleU1ThreadedSender::SendDataChunk(
    libusb_device_handle *handle,
    uint8_t *usb_data) {
  int transferred;
  int ret = m_adaptor->InterruptTransfer(
      handle, WRITE_ENDPOINT, reinterpret_cast<unsigned char*>(usb_data),
      DATABLOCK_SIZE, &transferred, URB_TIMEOUT_MS);
  if (ret) {
    OLA_WARN << "InterruptTransfer(): " << m_adaptor->ErrorCodeToString(ret)
             << ", transferred " << transferred << " / " << DATABLOCK_SIZE;
  }
  return ret == 0;
}

// DMXCProjectsNodleU1ThreadedReceiver
// -----------------------------------------------------------------------------

/*
 * Receives messages from a Nodle device in a separate thread.
 */
class DMXCProjectsNodleU1ThreadedReceiver: public ThreadedUsbReceiver {
 public:
  DMXCProjectsNodleU1ThreadedReceiver(ola::usb::LibUsbAdaptor *adaptor,
                                      libusb_device *usb_device,
                                      libusb_device_handle *handle,
                                      PluginAdaptor *plugin_adaptor)
      : ThreadedUsbReceiver(usb_device, handle, plugin_adaptor),
        m_adaptor(adaptor) {
  }

 private:
  ola::usb::LibUsbAdaptor* const m_adaptor;

  bool ReceiveBuffer(libusb_device_handle *handle,
                     DmxBuffer *buffer,
                     bool *buffer_updated);
  bool ReadDataChunk(libusb_device_handle *handle,
                     uint8_t *usb_data);
};

bool DMXCProjectsNodleU1ThreadedReceiver::ReceiveBuffer(
    libusb_device_handle *handle,
    DmxBuffer *buffer,
    bool *buffer_updated) {
  unsigned char usb_data[DATABLOCK_SIZE];

  if (ReadDataChunk(handle, usb_data)) {
    if (usb_data[0] < 16) {
      uint16_t start_offset = usb_data[0] * 32;
      buffer->SetRange(start_offset, &usb_data[1], 32);
      *buffer_updated = true;
    }
  }

  return true;
}

bool DMXCProjectsNodleU1ThreadedReceiver::ReadDataChunk(
    libusb_device_handle *handle,
    uint8_t *usb_data) {
  int transferred;
  int ret = m_adaptor->InterruptTransfer(
      handle, READ_ENDPOINT, reinterpret_cast<unsigned char*>(usb_data),
      DATABLOCK_SIZE, &transferred, URB_TIMEOUT_MS);
  if (ret && ret != LIBUSB_ERROR_TIMEOUT) {
    OLA_WARN << "InterruptTransfer(): " << m_adaptor->ErrorCodeToString(ret)
             << ", transferred " << transferred << " / " << DATABLOCK_SIZE;
  }
  return ret == 0;
}

// SynchronousDMXCProjectsNodleU1
// -----------------------------------------------------------------------------

SynchronousDMXCProjectsNodleU1::SynchronousDMXCProjectsNodleU1(
    ola::usb::LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    PluginAdaptor *plugin_adaptor,
    const string &serial,
    unsigned int mode)
    : DMXCProjectsNodleU1(adaptor, usb_device, plugin_adaptor, serial, mode),
      m_usb_device(usb_device) {
}

bool SynchronousDMXCProjectsNodleU1::Init() {
  libusb_device_handle *usb_handle = OpenDMXCProjectsNodleU1Widget(
      m_adaptor, m_usb_device);

  if (!usb_handle) {
    return false;
  }

  SetInterfaceMode(m_adaptor, usb_handle, m_mode);

  if (m_mode & OUTPUT_ENABLE_MASK) {  // output port active
    std::auto_ptr<DMXCProjectsNodleU1ThreadedSender> sender(
        new DMXCProjectsNodleU1ThreadedSender(m_adaptor, m_usb_device,
                                              usb_handle));
    if (!sender->Start()) {
      return false;
    }
    m_sender.reset(sender.release());
  }

  if (m_mode & INPUT_ENABLE_MASK) {  // input port active
    std::auto_ptr<DMXCProjectsNodleU1ThreadedReceiver> receiver(
        new DMXCProjectsNodleU1ThreadedReceiver(m_adaptor, m_usb_device,
                                                usb_handle, m_plugin_adaptor));
    if (!receiver->Start()) {
      return false;
    }
    m_receiver.reset(receiver.release());
  }

  return true;
}

bool SynchronousDMXCProjectsNodleU1::SendDMX(const DmxBuffer &buffer) {
  if (m_sender.get()) {
    return m_sender->SendDMX(buffer);
  } else {
    return false;
  }
}

void SynchronousDMXCProjectsNodleU1::SetDmxCallback(
    Callback0<void> *callback) {
  if (m_receiver.get()) {
    m_receiver->SetReceiveCallback(callback);
  } else {
    delete callback;
  }
}

const DmxBuffer &SynchronousDMXCProjectsNodleU1::GetDmxInBuffer() {
  return m_receiver->GetDmxInBuffer();
}

// DMXCProjectsNodleU1AsyncUsbReceiver
// -----------------------------------------------------------------------------
class DMXCProjectsNodleU1AsyncUsbReceiver : public AsyncUsbReceiver {
 public:
  DMXCProjectsNodleU1AsyncUsbReceiver(ola::usb::LibUsbAdaptor *adaptor,
                                      libusb_device *usb_device,
                                      PluginAdaptor *plugin_adaptor,
                                      unsigned int mode)
      : AsyncUsbReceiver(adaptor, usb_device, plugin_adaptor),
        m_mode(mode) {
  }

  ~DMXCProjectsNodleU1AsyncUsbReceiver() {
    CancelTransfer();
  }

  libusb_device_handle* SetupHandle() {
    libusb_device_handle *handle = OpenDMXCProjectsNodleU1Widget(m_adaptor,
                                                                 m_usb_device);
    if (handle) {
      SetInterfaceMode(m_adaptor, handle, m_mode);
    }
    return handle;
  }

  bool PerformTransfer();

  bool TransferCompleted(DmxBuffer *buffer, int transferred_size);

 private:
  unsigned int m_mode;
  uint8_t m_packet[DATABLOCK_SIZE];

  DISALLOW_COPY_AND_ASSIGN(DMXCProjectsNodleU1AsyncUsbReceiver);
};

bool DMXCProjectsNodleU1AsyncUsbReceiver::PerformTransfer() {
  FillInterruptTransfer(READ_ENDPOINT, m_packet,
                        DATABLOCK_SIZE, URB_TIMEOUT_MS);
  return (SubmitTransfer() == 0);
}

bool DMXCProjectsNodleU1AsyncUsbReceiver::TransferCompleted(
    DmxBuffer *buffer,
    int transferred_size) {
  if (m_packet[0] < 16 &&
      transferred_size >= static_cast<int>(DATABLOCK_SIZE)) {
    uint16_t start_offset = m_packet[0] * 32;
    buffer->SetRange(start_offset, &m_packet[1], 32);
    return true;
  }
  return false;
}

// DMXCProjectsNodleU1AsyncUsbSender
// -----------------------------------------------------------------------------
class DMXCProjectsNodleU1AsyncUsbSender : public AsyncUsbSender {
 public:
  DMXCProjectsNodleU1AsyncUsbSender(ola::usb::LibUsbAdaptor *adaptor,
                                    libusb_device *usb_device,
                                    unsigned int mode)
      : AsyncUsbSender(adaptor, usb_device),
        m_mode(mode),
        m_buffer_offset(0) {
    m_tx_buffer.Blackout();
  }

  ~DMXCProjectsNodleU1AsyncUsbSender() {
    CancelTransfer();
  }

  libusb_device_handle* SetupHandle() {
    libusb_device_handle *handle = OpenDMXCProjectsNodleU1Widget(m_adaptor,
                                                                 m_usb_device);
    if (handle) {
      SetInterfaceMode(m_adaptor, handle, m_mode);
    }
    return handle;
  }

  bool PerformTransfer(const DmxBuffer &buffer);

  void PostTransferHook();

 private:
  unsigned int m_mode;
  DmxBuffer m_tx_buffer;
  // This tracks where we are in m_tx_buffer. A value of 0 means we're at the
  // start of a DMX frame.
  unsigned int m_buffer_offset;
  uint8_t m_packet[DATABLOCK_SIZE];

  bool ContinueTransfer();

  bool SendInitialChunk(const DmxBuffer &buffer);

  bool SendChunk() {
    FillInterruptTransfer(WRITE_ENDPOINT, m_packet,
                          DATABLOCK_SIZE, URB_TIMEOUT_MS);
    return (SubmitTransfer() == 0);
  }

  DISALLOW_COPY_AND_ASSIGN(DMXCProjectsNodleU1AsyncUsbSender);
};

bool DMXCProjectsNodleU1AsyncUsbSender::PerformTransfer(
    const DmxBuffer &buffer) {
  if (m_buffer_offset == 0) {
    return SendInitialChunk(buffer);
  }
  // Otherwise we're part way through a transfer, do nothing.
  return true;
}

void DMXCProjectsNodleU1AsyncUsbSender::PostTransferHook() {
  if (m_buffer_offset < m_tx_buffer.Size()) {
    ContinueTransfer();
  } else if (m_buffer_offset >= m_tx_buffer.Size()) {
    // That was the last chunk.
    m_buffer_offset = 0;

    if (TransferPending()) {
      // If we have a pending transfer, the next chunk is going to be sent
      // once we return.
      m_tx_buffer.Reset();
    }
  }
}

bool DMXCProjectsNodleU1AsyncUsbSender::ContinueTransfer() {
  unsigned int length = 32;

  m_packet[0] = m_buffer_offset / 32;

  m_tx_buffer.GetRange(m_buffer_offset, m_packet + 1, &length);
  memset(m_packet + 1 + length, 0, 32 - length);
  m_buffer_offset += length;
  return (SendChunk() == 0);
}

bool DMXCProjectsNodleU1AsyncUsbSender::SendInitialChunk(
    const DmxBuffer &buffer) {
  unsigned int length = 32;

  m_tx_buffer.SetRange(0, buffer.GetRaw(), buffer.Size());

  m_packet[0] = 0;
  m_tx_buffer.GetRange(0, m_packet + 1, &length);
  memset(m_packet + 1 + length, 0, 32 - length);

  unsigned int slots_sent = length;
  if (slots_sent < m_tx_buffer.Size()) {
    // There are more frames to send.
    m_buffer_offset = slots_sent;
  }
  return (SendChunk() == 0);
}

// AsynchronousDMXCProjectsNodleU1
// -----------------------------------------------------------------------------

AsynchronousDMXCProjectsNodleU1::AsynchronousDMXCProjectsNodleU1(
    ola::usb::LibUsbAdaptor *adaptor,
    libusb_device *usb_device,
    PluginAdaptor *plugin_adaptor,
    const string &serial,
    unsigned int mode)
    : DMXCProjectsNodleU1(adaptor, usb_device, plugin_adaptor, serial, mode) {
  if (mode & OUTPUT_ENABLE_MASK) {  // output port active
    m_sender.reset(new DMXCProjectsNodleU1AsyncUsbSender(m_adaptor,
                                                         usb_device, mode));
  }

  if (mode & INPUT_ENABLE_MASK) {  // input port active
    m_receiver.reset(new DMXCProjectsNodleU1AsyncUsbReceiver(m_adaptor,
                                                             usb_device,
                                                             plugin_adaptor,
                                                             mode));
  }
}

bool AsynchronousDMXCProjectsNodleU1::Init() {
  bool ok = true;
  if (m_sender.get()) {
    ok &= m_sender->Init();
  }
  if (m_receiver.get()) {
    if (m_sender.get()) {
      // If we have a sender, use it's USB handle
      ok &= m_receiver->Init(m_sender->GetHandle());
    } else {
      ok &= m_receiver->Init();
    }
    if (ok) {
      m_receiver->Start();
    }
  }
  return ok;
}

bool AsynchronousDMXCProjectsNodleU1::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}

void AsynchronousDMXCProjectsNodleU1::SetDmxCallback(
    Callback0<void> *callback) {
  if (m_receiver.get()) {
    m_receiver->SetReceiveCallback(callback);
  } else {
    delete callback;
  }
}

const DmxBuffer &AsynchronousDMXCProjectsNodleU1::GetDmxInBuffer() {
  if (m_receiver.get()) {
    m_receiver->GetDmx(&m_buffer);
  }
  return m_buffer;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
