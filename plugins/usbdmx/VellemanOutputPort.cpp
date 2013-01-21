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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * VellemanOutputPort.cpp
 * Thread for the Velleman Output Port
 * Copyright (C) 2010 Simon Newton
 *
 * See the comments in VellemanOutputPort.h
 */

#include <string.h>
#include <sys/types.h>
#include <algorithm>
#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/VellemanOutputPort.h"
#include "plugins/usbdmx/VellemanDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;


/*
 * Create a new VellemanOutputPort object
 */
VellemanOutputPort::VellemanOutputPort(VellemanDevice *parent,
                                       unsigned int id,
                                       libusb_device *usb_device)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_chunk_size(8),  // the standard unit uses 8
      m_usb_device(usb_device),
      m_usb_handle(NULL) {
}


/*
 * Cleanup
 */
VellemanOutputPort::~VellemanOutputPort() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
}


/*
 * Start this thread
 */
bool VellemanOutputPort::Start() {
  libusb_device_handle *usb_handle;
  libusb_config_descriptor *config;

  if (libusb_open(m_usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open Velleman usb device";
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

  if (libusb_get_active_config_descriptor(m_usb_device, &config)) {
    OLA_WARN << "Could not get active config descriptor";
    libusb_close(usb_handle);
    return false;
  }

  // determine the max packet size, see
  // http://opendmx.net/index.php/Velleman_K8062_Upgrade
  if (config &&
      config->interface &&
      config->interface->altsetting &&
      config->interface->altsetting->endpoint) {
    uint16_t max_packet_size =
      config->interface->altsetting->endpoint->wMaxPacketSize;
    OLA_DEBUG << "Velleman K8062 max packet size is " << max_packet_size;
    if (max_packet_size == UPGRADED_CHUNK_SIZE)
      // this means the upgrade is present
      m_chunk_size = max_packet_size;
  }
  libusb_free_config_descriptor(config);

  if (libusb_claim_interface(usb_handle, INTERFACE)) {
    OLA_WARN << "Failed to claim Velleman usb device";
    libusb_close(usb_handle);
    return false;
  }

  m_usb_handle = usb_handle;
  bool ret = ola::thread::Thread::Start();
  if (!ret) {
    OLA_WARN << "pthread create failed";
    libusb_release_interface(m_usb_handle, INTERFACE);
    libusb_close(usb_handle);
    return false;
  }
  return true;
}


/*
 * Run this thread
 */
void *VellemanOutputPort::Run() {
  DmxBuffer buffer;
  if (!m_usb_handle)
    return NULL;

  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }

    {
      ola::thread::MutexLocker locker(&m_data_mutex);
      buffer.Set(m_buffer);
    }

    if (buffer.Size()) {
      if (!SendDMX(buffer)) {
        OLA_WARN << "Send failed, stopping thread...";
        break;
      }
    } else {
      // sleep for a bit
      usleep(40000);
    }
  }
  libusb_release_interface(m_usb_handle, 0);
  libusb_close(m_usb_handle);
  return NULL;
}


/*
 * Store the data in the shared buffer
 */
bool VellemanOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  ola::thread::MutexLocker locker(&m_data_mutex);
  m_buffer.Set(buffer);
  return true;
  (void) priority;
}


/**
 * Return the port description
 */
string VellemanOutputPort::Description() const {
  if (m_chunk_size == UPGRADED_CHUNK_SIZE)
    return "VX8062";
  else
    return "K8062";
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool VellemanOutputPort::SendDMX(const DmxBuffer &buffer) {
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

  if (!SendDataChunk(usb_data))
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
    if (!SendDataChunk(usb_data))
      return false;
  }

  // send the last channels
  if (m_chunk_size == UPGRADED_CHUNK_SIZE) {
    // if running in extended mode we can use the 6 message type to send
    // everything at once.
    usb_data[0] = 6;
    usb_data[1] = size - i;
    memcpy(usb_data + 2, data + i, size - i);
    if (!SendDataChunk(usb_data))
      return false;

  } else {
    // else we use the 3 message type to send one at a time
    for (; i != size; i++) {
      usb_data[0] = 3;
      usb_data[1] = data[i];
      if (!SendDataChunk(usb_data))
        return false;
    }
  }
  return true;
}


/*
 * Send 8 bytes to the usb device
 * @returns false if there was an error, true otherwise
 */
bool VellemanOutputPort::SendDataChunk(uint8_t *usb_data) {
  int transferred;
  int ret = libusb_interrupt_transfer(
      m_usb_handle,
      ENDPOINT,
      reinterpret_cast<unsigned char*>(usb_data),
      m_chunk_size,
      &transferred,
      URB_TIMEOUT_MS);
  if (ret)
    OLA_INFO << "USB return code was " << ret << ", transferred " <<
      transferred;
  return ret == 0;
}
}  // usbdmx
}  // plugin
}  // ola
