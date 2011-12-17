/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SunliteOutputPort.cpp
 * Thread for the Sunlite USBDMX2 Output Port
 * Copyright (C) 2010 Simon Newton
 *
 * See the comments in SunliteOutputPort.h
 */

#include <string.h>
#include <sys/types.h>

#include "ola/Logging.h"
#include "plugins/usbdmx/SunliteOutputPort.h"
#include "plugins/usbdmx/SunliteDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {


/*
 * Create a new SunliteOutputPort object
 */
SunliteOutputPort::SunliteOutputPort(SunliteDevice *parent,
                                     unsigned int id,
                                     libusb_device *usb_device)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_new_data(false),
      m_usb_device(usb_device),
      m_usb_handle(NULL) {
  InitPacket();
}


/*
 * Cleanup
 */
SunliteOutputPort::~SunliteOutputPort() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
}


/*
 * Start this thread
 */
bool SunliteOutputPort::Start() {
  libusb_device_handle *usb_handle;

  if (libusb_open(m_usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open Sunlite usb device";
    return false;
  }

  if (libusb_claim_interface(usb_handle, 0)) {
    OLA_WARN << "Failed to claim Sunlite usb device";
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
void *SunliteOutputPort::Run() {
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
bool SunliteOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  ola::thread::MutexLocker locker(&m_data_mutex);
  m_buffer.Set(buffer);
  m_new_data = true;
  return true;
  (void) priority;
}


/*
 * Setup the packet we send to the device.
 *
 * The packet is divided into 26 chunks of 32 bytes each. Each chunk contains
 * the data for 20 channels (except the last one which has 12 channels of data)
 */
void SunliteOutputPort::InitPacket() {
  // zero everything
  memset(m_packet, 0, SUNLITE_PACKET_SIZE);

  for (unsigned int chunk = 0; chunk < CHUNKS_PER_PACKET; ++chunk) {
    unsigned int i = chunk * CHUNK_SIZE;  // index into the packet
    unsigned int channel = chunk * CHANNELS_PER_CHUNK;

    m_packet[i] = 0x80;
    m_packet[i+1] = channel / 2;
    m_packet[i+2] = 0x84;
    m_packet[i+7] = channel / 2 + 2;
    m_packet[i+8] = 0x84;
    m_packet[i+13] = channel / 2 + 4;
    if (chunk < CHUNKS_PER_PACKET - 1) {
      m_packet[i+14] = 0x84;
      m_packet[i+19] = channel / 2 + 6;
      m_packet[i+20] = 0x84;
      m_packet[i+25] = channel / 2 + 8;
      m_packet[i+26] = 0x04;
      m_packet[i+31] = 0x00;
    } else {
      // the last m_packet is short
      m_packet[i+14] = 0x04;
    }
  }
}


/*
 * Send DMX to the widget
 */
bool SunliteOutputPort::SendDMX(const DmxBuffer &buffer) {
  for (unsigned int i = 0; i < buffer.Size(); i++)
    m_packet[(i / CHANNELS_PER_CHUNK) * CHUNK_SIZE +
             ((i / 4) % 5) * 6 + 3 + (i % 4)] = buffer.Get(i);

  int transferred;
  int r = libusb_bulk_transfer(
      m_usb_handle,
      ENDPOINT,
      (unsigned char*) m_packet,
      SUNLITE_PACKET_SIZE,
      &transferred,
      TIMEOUT);
  if (transferred != SUNLITE_PACKET_SIZE)
    // not sure if this is fatal or not
    OLA_WARN << "Sunlite driver failed to transfer all data";
  return r == 0;
}
}  // usbdmx
}  // plugin
}  // ola
