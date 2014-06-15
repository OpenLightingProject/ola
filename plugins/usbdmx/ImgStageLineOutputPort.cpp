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
 * ImgStageLineOutputPort.cpp
 * Thread for the img Stage Line DMX-1USB Output Port
 * Copyright (C) 2014 Peter Newman
 *
 * See the comments in ImgStageLineOutputPort.h
 */

#include <string.h>
#include <sys/types.h>
#include <algorithm>

#include "ola/Logging.h"
#include "plugins/usbdmx/ImgStageLineOutputPort.h"
#include "plugins/usbdmx/ImgStageLineDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {


/*
 * Create a new ImgStageLineOutputPort object
 */
ImgStageLineOutputPort::ImgStageLineOutputPort(ImgStageLineDevice *parent,
                                               unsigned int id,
                                               libusb_device *usb_device)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_new_data(false),
      m_usb_device(usb_device),
      m_usb_handle(NULL) {
}


/*
 * Cleanup
 */
ImgStageLineOutputPort::~ImgStageLineOutputPort() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
}


/*
 * Start this thread
 */
bool ImgStageLineOutputPort::Start() {
  libusb_device_handle *usb_handle;

  if (libusb_open(m_usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open img USB device";
    return false;
  }

  if (libusb_claim_interface(usb_handle, 0)) {
    OLA_WARN << "Failed to claim img USB device";
    libusb_close(usb_handle);
    return false;
  }

  m_usb_handle = usb_handle;
  bool ret = ola::thread::Thread::Start();
  if (!ret) {
    OLA_WARN << "pthread create failed";
    libusb_release_interface(m_usb_handle, 0);
    libusb_close(usb_handle);
    return false;
  }
  return true;
}


/*
 * Run this thread
 */
void *ImgStageLineOutputPort::Run() {
  DmxBuffer buffer;
  bool new_data;

  if (!m_usb_handle)
    return NULL;

  while (true) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }

    {
      ola::thread::MutexLocker locker(&m_data_mutex);
      buffer.Set(m_buffer);
      new_data = m_new_data;
      m_new_data = false;
    }

    if (new_data) {
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
bool ImgStageLineOutputPort::WriteDMX(const DmxBuffer &buffer,
                                      uint8_t priority) {
  ola::thread::MutexLocker locker(&m_data_mutex);
  m_buffer.Set(buffer);
  m_new_data = true;
  return true;
  (void) priority;
}


/*
 * Send DMX to the widget
 */
bool ImgStageLineOutputPort::SendDMX(const DmxBuffer &buffer) {
  bool success = false;

  for (unsigned int i = 0;
       i < DMX_MAX_TRANSMIT_CHANNELS;
       i = i + CHANNELS_PER_PACKET) {
    // zero everything
    memset(m_packet, 0, IMGSTAGELINE_PACKET_SIZE);

    if (i == 0) {
      m_packet[0] = CHANNEL_HEADER_LOW;
    } else if (i == CHANNELS_PER_PACKET) {
      m_packet[0] = CHANNEL_HEADER_HIGH;
    } else {
      OLA_FATAL << "Unknown channel value " << i << ", couldn't find channel "
                   "header value";
    }

    // Copy the data if there is some, otherwise we'll just send a packet of
    // zeros for the channel values
    if ((buffer.Size() - i) > 0) {
      unsigned int channels = std::min((unsigned int) CHANNELS_PER_PACKET,
                                       (buffer.Size() - i));

      buffer.GetRange(i, &m_packet[1], &channels);
    }

    int transferred;
    int r = libusb_bulk_transfer(
        m_usb_handle,
        ENDPOINT,
        (unsigned char*) m_packet,
        IMGSTAGELINE_PACKET_SIZE,
        &transferred,
        TIMEOUT);
    if (transferred != IMGSTAGELINE_PACKET_SIZE) {
      // not sure if this is fatal or not
      OLA_WARN << "img driver failed to transfer all data";
    }
    success = (r == 0);
    if (!success) {
      return success;
    }
  }
  return success;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
