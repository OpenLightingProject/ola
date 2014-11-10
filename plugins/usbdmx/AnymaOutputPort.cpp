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
 * AnymaOutputPort.cpp
 * Thread for the Anyma Output Port
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <sys/types.h>
#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/AnymaOutputPort.h"
#include "plugins/usbdmx/AnymaDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;


/*
 * Create a new AnymaOutputPort object
 */
AnymaOutputPort::AnymaOutputPort(AnymaDevice *parent,
                                 unsigned int id,
                                 libusb_device_handle *usb_handle,
                                 const string &serial)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_serial(serial),
      m_usb_handle(usb_handle) {
}


/*
 * Cleanup
 */
AnymaOutputPort::~AnymaOutputPort() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
}


/*
 * Start this thread
 */
bool AnymaOutputPort::Start() {
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
void *AnymaOutputPort::Run() {
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
bool AnymaOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  ola::thread::MutexLocker locker(&m_data_mutex);
  m_buffer.Set(buffer);
  return true;
  (void) priority;
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool AnymaOutputPort::SendDMX(const DmxBuffer &buffer) {
  int r = libusb_control_transfer(m_usb_handle,
          LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
          LIBUSB_ENDPOINT_OUT,
          UDMX_SET_CHANNEL_RANGE,
          buffer.Size(),
          0,
          // the suck
          const_cast<unsigned char*>(buffer.GetRaw()),
          buffer.Size(),
          URB_TIMEOUT_MS);
  // Sometimes we get PIPE errors here, those are non-fatal
  return r > 0 || r == LIBUSB_ERROR_PIPE;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
