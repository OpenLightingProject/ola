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
 * VellemanOutputPort.cpp
 * Thread for the Velleman Output Port
 * Copyright (C) 2010 Simon Newton
 *
 * See the comments in VellemanOutputPort.h
 */

#include <string.h>
#include <sys/types.h>
#include <string>

#include "ola/Logging.h"
#include "plugins/usbdmx/VellemanOutputPort.h"
#include "plugins/usbdmx/VellemanDevice.h"


namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;


/*
 * Called by the new thread
 */
void *thread_run(void *d) {
  VellemanOutputPort *port = reinterpret_cast<VellemanOutputPort*>(d);
  port->Run();
}


/*
 * Create a new VellemanOutputPort object
 */
VellemanOutputPort::VellemanOutputPort(VellemanDevice *parent,
                                       unsigned int id,
                                       libusb_device *usb_device)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_usb_device(usb_device),
      m_usb_handle(NULL),
      m_thread_id(0) {
  pthread_mutex_init(&m_data_mutex, NULL);
  pthread_mutex_init(&m_term_mutex, NULL);
}


/*
 * Cleanup
 */
VellemanOutputPort::~VellemanOutputPort() {
  pthread_mutex_lock(&m_term_mutex);
  m_term = true;
  pthread_mutex_unlock(&m_term_mutex);
  if (m_thread_id)
    pthread_join(m_thread_id, NULL);

  pthread_mutex_destroy(&m_term_mutex);
  pthread_mutex_destroy(&m_data_mutex);
}


/*
 * Start this thread
 */
bool VellemanOutputPort::Start() {
  libusb_device_handle *usb_handle;

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
  if (libusb_set_configuration(usb_handle, CONFIGURATION)) {
    OLA_WARN << "Velleman set config failed";
    libusb_close(usb_handle);
    return false;
  }

  if (libusb_claim_interface(usb_handle, 0)) {
    OLA_WARN << "Failed to claim Velleman usb device";
    libusb_close(usb_handle);
    return false;
  }

  m_usb_handle = usb_handle;
  int ret = pthread_create(&m_thread_id,
                           NULL,
                           ola::plugin::usbdmx::thread_run,
                           reinterpret_cast<void*>(this));
  if (ret) {
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
void *VellemanOutputPort::Run() {
  DmxBuffer buffer;
  if (!m_usb_handle)
    return NULL;

  while (1) {
    pthread_mutex_lock(&m_term_mutex);
    if (m_term) {
      pthread_mutex_unlock(&m_term_mutex);
      break;
    }
    pthread_mutex_unlock(&m_term_mutex);

    pthread_mutex_lock(&m_data_mutex);
    buffer.Set(m_buffer);
    pthread_mutex_unlock(&m_data_mutex);

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
  pthread_mutex_lock(&m_data_mutex);
  m_buffer.Set(buffer);
  pthread_mutex_unlock(&m_data_mutex);
  return true;
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool VellemanOutputPort::SendDMX(const DmxBuffer &buffer) {
  unsigned char usb_data[VELLEMAN_USB_CHUNK_SIZE];
  unsigned int size = buffer.Size();
  const uint8_t *data = buffer.GetRaw();
  unsigned int i = 0;
  unsigned int n;

  memset(usb_data, 0, sizeof(usb_data));

  for (n = 0; n < MAX_COMPRESSED_CHANNELS && n < size - COMPRESSED_CHANNEL_COUNT && !data[n]; n++);
  usb_data[0] = 4;
  usb_data[1] = n + 1;  // include start code
  memcpy(usb_data + 2, data + n, COMPRESSED_CHANNEL_COUNT);
  if (!SendDataChunk(usb_data))
    return false;
  i += n + COMPRESSED_CHANNEL_COUNT;

  while (i < size - CHANNEL_COUNT) {
    for (n = 0;
         n < MAX_COMPRESSED_CHANNELS && n + i < size - COMPRESSED_CHANNEL_COUNT && !data[i + n];
         n++);
    if (n) {
      // we have leading zeros
      usb_data[0] = 5;
      usb_data[1] = n;
      memcpy(usb_data + 2, data + i + n, COMPRESSED_CHANNEL_COUNT);
      i += n + COMPRESSED_CHANNEL_COUNT;
    } else {
      usb_data[0] = 2;
      memcpy(usb_data + 1, data + i, CHANNEL_COUNT);
      i += CHANNEL_COUNT;
    }
    if (!SendDataChunk(usb_data))
      return false;
  }

  // send the last channels
  for (;i != size; i++) {
    usb_data[0] = 3;
    usb_data[1] = data[i];
    if (!SendDataChunk(usb_data))
      return false;
  }
  return true;
}


/*
 * Send 8 bytes to the usb device
 * @returns false if there was an error, true otherwise
 */
bool VellemanOutputPort::SendDataChunk(uint8_t *usb_data) {
  OLA_DEBUG << "Sending " <<
    (int) usb_data[0] << "," <<
    (int) usb_data[1] << "," <<
    (int) usb_data[2] << "," <<
    (int) usb_data[3] << "," <<
    (int) usb_data[4] << "," <<
    (int) usb_data[5] << "," <<
    (int) usb_data[6] << "," <<
    (int) usb_data[7] << ",";
  int transferred;
  int ret = libusb_interrupt_transfer(m_usb_handle,
                                      ENDPOINT,
                                      reinterpret_cast<unsigned char*>(usb_data),
                                      VELLEMAN_USB_CHUNK_SIZE,
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
