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


const char AnymaOutputPort::EXPECTED_MANUFACTURER[] = "www.anyma.ch";
const char AnymaOutputPort::EXPECTED_PRODUCT[] = "uDMX";

/*
 * Create a new AnymaOutputPort object
 */
AnymaOutputPort::AnymaOutputPort(AnymaDevice *parent,
                                 unsigned int id,
                                 libusb_device *usb_device)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_serial(""),
      m_usb_device(usb_device),
      m_usb_handle(NULL) {
  pthread_mutex_init(&m_data_mutex, NULL);
  pthread_mutex_init(&m_term_mutex, NULL);
}


/*
 * Cleanup
 */
AnymaOutputPort::~AnymaOutputPort() {
  pthread_mutex_lock(&m_term_mutex);
  m_term = true;
  pthread_mutex_unlock(&m_term_mutex);
  Join();

  pthread_mutex_destroy(&m_term_mutex);
  pthread_mutex_destroy(&m_data_mutex);
}


/*
 * Start this thread
 */
bool AnymaOutputPort::Start() {
  libusb_device_handle *usb_handle;
  struct libusb_device_descriptor device_descriptor;
  libusb_get_device_descriptor(m_usb_device, &device_descriptor);

  if (libusb_open(m_usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open Anyma usb device";
    return false;
  }

  // this device only has one configuration
  /*
  if (libusb_set_configuration(usb_handle, CONFIGURATION)) {
    OLA_WARN << "Anyma set config failed";
    libusb_close(usb_handle);
    return false;
  }
*/
  string data;
  if (!GetDescriptorString(usb_handle, device_descriptor.iManufacturer,
                           &data)) {
    OLA_INFO << "Failed to get manufactuer name";
    libusb_close(usb_handle);
    return false;
  }

  if (data != EXPECTED_MANUFACTURER) {
    OLA_INFO << "Manufacturer mismatch: " << EXPECTED_MANUFACTURER << " != " <<
      data;
    libusb_close(usb_handle);
    return false;
  }

  if (!GetDescriptorString(usb_handle, device_descriptor.iProduct, &data)) {
    OLA_INFO << "Failed to get product name";
    libusb_close(usb_handle);
    return false;
  }

  if (data != EXPECTED_PRODUCT) {
    OLA_INFO << "Product mismatch: " << EXPECTED_PRODUCT << " != " << data;
    libusb_close(usb_handle);
    return false;
  }

  if (!GetDescriptorString(usb_handle, device_descriptor.iSerialNumber,
                           &data)) {
    OLA_INFO << "Failed to get serial number";
    libusb_close(usb_handle);
    return false;
  }

  m_serial = data;

  if (libusb_claim_interface(usb_handle, 0)) {
    OLA_WARN << "Failed to claim Anyma usb device";
    libusb_close(usb_handle);
    return false;
  }

  m_usb_handle = usb_handle;
  bool ret = OlaThread::Start();
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
void *AnymaOutputPort::Run() {
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
bool AnymaOutputPort::WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
  pthread_mutex_lock(&m_data_mutex);
  m_buffer.Set(buffer);
  pthread_mutex_unlock(&m_data_mutex);
  return true;
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


/*
 * Return a string descriptor
 * @param usb_handle the usb handle to the device
 * @param desc_index the index of the descriptor
 * @param data where to store the output string
 * @returns true if we got the value, false otherwise
 */
bool AnymaOutputPort::GetDescriptorString(libusb_device_handle *usb_handle,
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
    return false;
  }
  data->assign(reinterpret_cast<char*>(buffer));
  return true;
}
}  // usbdmx
}  // plugin
}  // ola
