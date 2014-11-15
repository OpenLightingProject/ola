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
 * VellemanWidget.cpp
 * The synchronous and asynchronous Velleman widgets.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/VellemanWidget.h"

#include <unistd.h>
#include <algorithm>
#include <string>

#include "ola/Logging.h"
#include "ola/Constants.h"
#include "plugins/usbdmx/LibUsbHelper.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

namespace {

static const unsigned char ENDPOINT = 0x01;
// 25ms seems to be about the shortest we can go
static const unsigned int URB_TIMEOUT_MS = 25;
static const int CONFIGURATION = 1;
static const int INTERFACE = 0;
static const unsigned int UPGRADED_CHUNK_SIZE = 64;

/*
 * Called by the AsynchronousSunliteWidget when the transfer completes.
void AsyncCallback(struct libusb_transfer *transfer) {
  AsynchronousVellemanWidget *widget =
      reinterpret_cast<AsynchronousVellemanWidget*>(transfer->user_data);
  widget->TransferComplete(transfer);
}
 */

}  // namespace

/*
 * Sends messages to a Velleman device in a separate thread.
 */
class VellemanThreadedSender: public ThreadedUsbSender {
 public:
  VellemanThreadedSender(libusb_device *usb_device,
                         libusb_device_handle *handle,
                         unsigned int chunk_size)
      : ThreadedUsbSender(usb_device, handle),
        m_chunk_size(chunk_size) {
  }

 private:
  const unsigned int m_chunk_size;

  bool TransmitBuffer(libusb_device_handle *handle,
                      const DmxBuffer &buffer);
  bool SendDataChunk(libusb_device_handle *handle, uint8_t *usb_data,
                     unsigned int chunk_size);
};

bool VellemanThreadedSender::TransmitBuffer(libusb_device_handle *handle,
                                            const DmxBuffer &buffer) {
  unsigned char usb_data[m_chunk_size];
  unsigned int size = buffer.Size();
  const uint8_t *data = buffer.GetRaw();
  unsigned int i = 0;
  unsigned int n;

  // this could be up to 254 for the standard interface but then the shutdown
  // process gets wacky. Limit it to 100 for the standard and 255 for the
  // extended.
  unsigned int max_compressed_channels = m_chunk_size == UPGRADED_CHUNK_SIZE ?
    254 : 100;
  unsigned int compressed_channel_count = m_chunk_size - 2;
  unsigned int channel_count = m_chunk_size - 1;

  memset(usb_data, 0, sizeof(usb_data));

  if (m_chunk_size == UPGRADED_CHUNK_SIZE && size <= m_chunk_size - 2) {
    // if the upgrade is present and we can fit the data in a single packet
    // use the 7 message type
    usb_data[0] = 7;
    usb_data[1] = size;  // number of channels in packet
    memcpy(usb_data + 2, data, std::min(size, m_chunk_size - 2));
  } else {
    // otherwise use 4 to signal the start of frame
    for (n = 0;
         n < max_compressed_channels && n < size - compressed_channel_count
         && !data[n];
         n++) {
    }
    usb_data[0] = 4;
    usb_data[1] = n + 1;  // include start code
    memcpy(usb_data + 2, data + n, compressed_channel_count);
    i += n + compressed_channel_count;
  }

  if (!SendDataChunk(handle, usb_data, m_chunk_size))
    return false;

  while (i < size - channel_count) {
    for (n = 0;
         n < max_compressed_channels && n + i < size - compressed_channel_count
           && !data[i + n];
         n++) {
    }
    if (n) {
      // we have leading zeros
      usb_data[0] = 5;
      usb_data[1] = n;
      memcpy(usb_data + 2, data + i + n, compressed_channel_count);
      i += n + compressed_channel_count;
    } else {
      usb_data[0] = 2;
      memcpy(usb_data + 1, data + i, channel_count);
      i += channel_count;
    }
    if (!SendDataChunk(handle, usb_data, m_chunk_size))
      return false;
  }

  // send the last channels
  if (m_chunk_size == UPGRADED_CHUNK_SIZE) {
    // if running in extended mode we can use the 6 message type to send
    // everything at once.
    usb_data[0] = 6;
    usb_data[1] = size - i;
    memcpy(usb_data + 2, data + i, size - i);
    if (!SendDataChunk(handle, usb_data, m_chunk_size))
      return false;

  } else {
    // else we use the 3 message type to send one at a time
    for (; i != size; i++) {
      usb_data[0] = 3;
      usb_data[1] = data[i];
      if (!SendDataChunk(handle, usb_data, m_chunk_size))
        return false;
    }
  }
  return true;
}

bool VellemanThreadedSender::SendDataChunk(libusb_device_handle *handle,
                                           uint8_t *usb_data,
                                           unsigned int chunk_size) {
  int transferred;
  int ret = libusb_interrupt_transfer(
      handle,
      ENDPOINT,
      reinterpret_cast<unsigned char*>(usb_data),
      chunk_size,
      &transferred,
      URB_TIMEOUT_MS);
  if (ret) {
    OLA_INFO << "USB return code was " << ret << ", transferred "
             << transferred;
  }
  return ret == 0;
}


SynchronousVellemanWidget::SynchronousVellemanWidget(
    libusb_device *usb_device)
    : m_usb_device(usb_device) {
}

bool SynchronousVellemanWidget::Init() {
  libusb_device_handle *usb_handle;

  bool ok = LibUsbHelper::OpenDevice(m_usb_device, &usb_handle);
  if (!ok) {
    return false;
  }

  if (libusb_kernel_driver_active(usb_handle, 0)) {
    if (libusb_detach_kernel_driver(usb_handle, 0)) {
      OLA_WARN << "Failed to detach kernel driver";
      libusb_close(usb_handle);
      return false;
    }
  }

  // this device only has one configuration
  int ret_code = libusb_set_configuration(usb_handle, CONFIGURATION);
  if (ret_code) {
    OLA_WARN << "Velleman set config failed, with libusb error code " <<
      ret_code;
    libusb_close(usb_handle);
    return false;
  }

  libusb_config_descriptor *config;
  if (libusb_get_active_config_descriptor(m_usb_device, &config)) {
    OLA_WARN << "Could not get active config descriptor";
    libusb_close(usb_handle);
    return false;
  }

  // determine the max packet size, see
  // http://opendmx.net/index.php/Velleman_K8062_Upgrade
  // The standard size is 8.
  unsigned int chunk_size = 8;
  if (config &&
      config->interface &&
      config->interface->altsetting &&
      config->interface->altsetting->endpoint) {
    uint16_t max_packet_size =
      config->interface->altsetting->endpoint->wMaxPacketSize;
    OLA_DEBUG << "Velleman K8062 max packet size is " << max_packet_size;
    if (max_packet_size == UPGRADED_CHUNK_SIZE)
      // this means the upgrade is present
      chunk_size = max_packet_size;
  }
  libusb_free_config_descriptor(config);

  if (libusb_claim_interface(usb_handle, INTERFACE)) {
    OLA_WARN << "Failed to claim Velleman usb device";
    libusb_close(usb_handle);
    return false;
  }

  std::auto_ptr<VellemanThreadedSender> sender(
      new VellemanThreadedSender(m_usb_device, usb_handle, chunk_size));
  if (!sender->Start()) {
    return false;
  }
  m_sender.reset(sender.release());
  return true;
}

bool SynchronousVellemanWidget::SendDMX(const DmxBuffer &buffer) {
  return m_sender.get() ? m_sender->SendDMX(buffer) : false;
}


AsynchronousVellemanWidget::AsynchronousVellemanWidget(
    libusb_device *usb_device)
    : VellemanWidget(),
      m_usb_device(usb_device),
      m_usb_handle(NULL),
      m_control_setup_buffer(NULL),
      m_transfer_state(IDLE) {
  m_control_setup_buffer =
    new uint8_t[LIBUSB_CONTROL_SETUP_SIZE + DMX_UNIVERSE_SIZE];

  m_transfer = libusb_alloc_transfer(0);
  libusb_ref_device(usb_device);
}

AsynchronousVellemanWidget::~AsynchronousVellemanWidget() {
  bool canceled = false;
  OLA_INFO << "AsynchronousVellemanWidget shutdown";
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
  delete[] m_control_setup_buffer;
  libusb_close(m_usb_handle);
  libusb_unref_device(m_usb_device);
}

bool AsynchronousVellemanWidget::Init() {
  bool ok = LibUsbHelper::OpenDeviceAndClaimInterface(
      m_usb_device, 0, &m_usb_handle);
  if (!ok) {
    return false;
  }
  return true;
}

bool AsynchronousVellemanWidget::SendDMX(const DmxBuffer &buffer) {
  OLA_INFO << "Call to AsynchronousVellemanWidget::SendDMX";

  if (!m_usb_handle) {
    OLA_WARN << "AsynchronousVellemanWidget hasn't been initialized";
    return false;
  }

  (void) buffer;
  return false;
  /*
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_transfer_state != IDLE) {
    return true;
  }

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

  libusb_fill_control_transfer(
      m_transfer,
      m_usb_handle,
      m_control_setup_buffer,
      &AsyncCallback,
      this,
      URB_TIMEOUT_MS);

  int ret = libusb_submit_transfer(m_transfer);
  if (ret) {
    OLA_WARN << "libusb_submit_transfer returned " << libusb_error_name(ret);
    return false;
  }
  OLA_INFO << "submit ok";
  m_transfer_state = IN_PROGRESS;
  return true;
  */
}

void AsynchronousVellemanWidget::TransferComplete(
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
