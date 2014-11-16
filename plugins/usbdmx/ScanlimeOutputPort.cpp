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
  unsigned int index = 0;
  memset(&packet, 0, sizeof(packet));

  for (unsigned int entry = 0; entry < (512*3); entry++) {
    unsigned int packet_entry = ((entry % (21 * 3)) + 1);
    OLA_DEBUG << "Working on entry " << entry << " with packet index " << index << ", packet entry " << packet_entry;
    if (entry < buffer.Size()) {
      OLA_DEBUG << "Using channel " << entry << " with val " << static_cast<unsigned int>(buffer.Get(entry));
      packet[packet_entry] = buffer.Get(entry);
    }
    if ((packet_entry == (21 * 3)) || (entry == ((512*3) - 1))) {
      if (entry == ((512*3) - 1)) {
        OLA_DEBUG << "Setting final flag on packet";
        packet[0] |= (1 << 5);  // Final
      }
      packet[0] |= index;

      // Send the data
      int txed = 0;

      int r = libusb_bulk_transfer(m_usb_handle,
                                   1,
                                   packet,
                                   sizeof(packet),
                                   &txed,
                                   2000);

      OLA_INFO << "Packet " << index << " transferred " << txed << " bytes";

      if (r != 0) {
        return false;
      }

      // Get ready for the next packet
      index++;
      memset(&packet, 0, sizeof(packet));
    }
  }
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
