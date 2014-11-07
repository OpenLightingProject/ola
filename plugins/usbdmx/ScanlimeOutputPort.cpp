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
 * ScanlimeOutputPort.cpp
 * Thread for the Scanlime Output Port
 * Copyright (C) 2014 Peter Newman
 */

#include <string.h>
#include <sys/types.h>
#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/ScanlimeOutputPort.h"
#include "plugins/usbdmx/ScanlimeDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;


/*
 * Create a new ScanlimeOutputPort object
 */
ScanlimeOutputPort::ScanlimeOutputPort(ScanlimeDevice *parent,
                                       unsigned int id,
                                       libusb_device_handle *usb_handle)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_usb_handle(usb_handle) {
}


/*
 * Cleanup
 */
ScanlimeOutputPort::~ScanlimeOutputPort() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
}


/*
 * Start this thread
 */
bool ScanlimeOutputPort::Start() {
  bool ret = ola::thread::Thread::Start();
  if (!ret) {
    OLA_WARN << "Failed to start sender thread";
    libusb_release_interface(m_usb_handle, 0);
    libusb_close(m_usb_handle);
    return false;
  }
  return true;
}


/*
 * Run this thread
 */
void *ScanlimeOutputPort::Run() {
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
bool ScanlimeOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  ola::thread::MutexLocker locker(&m_data_mutex);
  m_buffer.Set(buffer);
  return true;
  (void) priority;
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool ScanlimeOutputPort::SendDMX(const DmxBuffer &buffer) {
  uint8_t packet[64];
  memset(&packet, 0, sizeof(packet));
  packet[0] = 0x80;
  packet[1] = (1 << 2);

  if (buffer.Get(0) > 127) {
    packet[1] |= (1 << 3);
  }

  int txed = 0;

  int r = libusb_bulk_transfer(m_usb_handle,
                               1,
                               packet,
                               sizeof(packet),
                               &txed,
                               2000);

  OLA_INFO << "Transferred " << txed << " bytes";

  return r == 0;
}


/*
 * Return a string descriptor
 * @param usb_handle the usb handle to the device
 * @param desc_index the index of the descriptor
 * @param data where to store the output string
 * @returns true if we got the value, false otherwise
 */
bool ScanlimeOutputPort::GetDescriptorString(libusb_device_handle *usb_handle,
                                             uint8_t desc_index,
                                             string *data) {
  enum { buffer_size = 32 };  // static arrays FTW!
  unsigned char buffer[buffer_size];
  int r = libusb_get_string_descriptor_ascii(
      usb_handle,
      desc_index,
      buffer,
      buffer_size);

  if (r <= 0) {
    OLA_INFO << "libusb_get_string_descriptor_ascii returned " << r;
    return false;
  }
  data->assign(reinterpret_cast<char*>(buffer));
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
