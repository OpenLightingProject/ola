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
 * EuroliteProOutputPort.cpp
 * Thread for the EurolitePro Output Port
 * Copyright (C) 2011 Simon Newton & Harry F
 * Eurolite Pro USB DMX   ArtNo. 51860120
 */

#include <string.h>
#include <string>

#include "ola/BaseTypes.h"
#include "ola/Logging.h"
#include "plugins/usbdmx/EuroliteProOutputPort.h"
#include "plugins/usbdmx/EuroliteProDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

const char EuroliteProOutputPort::EXPECTED_MANUFACTURER[] = "Eurolite";
const char EuroliteProOutputPort::EXPECTED_PRODUCT[] = "Eurolite DMX512 Pro";
const uint8_t EuroliteProOutputPort::DMX_LABEL;
const unsigned char EuroliteProOutputPort::ENDPOINT;
const unsigned int EuroliteProOutputPort::UDMX_SET_CHANNEL_RANGE;
const unsigned int EuroliteProOutputPort::URB_TIMEOUT_MS;

/*
 * Create a new EuroliteProOutputPort object
 */
EuroliteProOutputPort::EuroliteProOutputPort(EuroliteProDevice *parent,
                                             unsigned int id,
                                             libusb_device *usb_device)
    : BasicOutputPort(parent, id),
      m_term(false),
      m_serial(""),
      m_usb_device(usb_device),
      m_usb_handle(NULL) {
}


/*
 * Cleanup
 */
EuroliteProOutputPort::~EuroliteProOutputPort() {
  {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }
  Join();
}


/*
 * Start this thread
 */
bool EuroliteProOutputPort::Start() {
  libusb_device_handle *usb_handle;
  struct libusb_device_descriptor device_descriptor;
  libusb_get_device_descriptor(m_usb_device, &device_descriptor);

  if (libusb_open(m_usb_device, &usb_handle)) {
    OLA_WARN << "Failed to open Eurolite usb device";
    return false;
  }

  string data;
  if (!GetDescriptorString(usb_handle, device_descriptor.iManufacturer,
                           &data)) {
    OLA_INFO << "Failed to get manufacturer name";
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

  bool ok = LocateInterface();

  if (!ok) {
    libusb_close(usb_handle);
    return false;
  }

  // The Eurolite doesn't have a serial number, so instead we use the device &
  // bus number.
  // TODO: check if this supports the SERIAL NUMBER label and use that instead.

  // There is no Serialnumber--> work around: bus+device number
  int bus_number = libusb_get_bus_number(m_usb_device);
  int device_address = libusb_get_device_address(m_usb_device);

  OLA_INFO << "Bus_number: " <<  bus_number << ", Device_address: " <<
    device_address;

  std::stringstream str;
  str << bus_number << "-" << device_address;
  m_serial = str.str();

  int error = libusb_claim_interface(usb_handle, m_interface_number);

  if (error) {
    if (error == LIBUSB_ERROR_BUSY) {
      OLA_WARN << "Eurolite device in use by another program";
    } else {
      OLA_WARN << "Failed to claim Eurolite usb interface, error: " << error;
    }
    libusb_close(usb_handle);
    return false;
  }

  m_usb_handle = usb_handle;
  bool ret = ola::thread::Thread::Start();
  if (!ret) {
    OLA_WARN << "pthread create failed";
    libusb_release_interface(m_usb_handle, m_interface_number);
    libusb_close(usb_handle);
    return false;
  }
  return true;
}


/*
 * The main loop for the sender thread.
 */
void *EuroliteProOutputPort::Run() {
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
        OLA_WARN << "Send bufferfailed, stopping thread...";
        break;
      }
    } else {
      // sleep for a bit
      usleep(40000);
    }
  }
  libusb_release_interface(m_usb_handle, m_interface_number);
  libusb_close(m_usb_handle);
  return NULL;
}


/*
 * Store the data in the shared buffer
 */
bool EuroliteProOutputPort::WriteDMX(const DmxBuffer &buffer,
                                     uint8_t priority) {
  ola::thread::MutexLocker locker(&m_data_mutex);
  m_buffer.Set(buffer);
  return true;
  (void) priority;
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool EuroliteProOutputPort::SendDMX(const DmxBuffer &buffer) {
  uint8_t usb_data[FRAME_SIZE];
  unsigned int frame_size = buffer.Size();

  // header
  usb_data[0] = 0x7E;   // Start message delimiter
  usb_data[1] = DMX_LABEL;      // Label
  usb_data[4] = DMX512_START_CODE;
  buffer.Get(usb_data + 5, &frame_size);
  usb_data[2] = (DMX_UNIVERSE_SIZE + 1) & 0xff;  // Data length LSB.
  usb_data[3] = ((DMX_UNIVERSE_SIZE + 1) >> 8);  // Data length MSB
  memset(usb_data + 5 + frame_size, 0, DMX_UNIVERSE_SIZE - frame_size);
  usb_data[FRAME_SIZE - 1] =  0xE7;  // End message delimiter

  int transferred = 0;
  int ret = libusb_bulk_transfer(
        m_usb_handle,
        ENDPOINT,
        usb_data,
        FRAME_SIZE,
        &transferred,
        URB_TIMEOUT_MS);

  if (ret)
    OLA_INFO << "return code was: " << ret << ", transferred bytes  " <<
      transferred;
  return ret == 0;
}


/*
 * Return a string descriptor
 * @param usb_handle the usb handle to the device
 * @param desc_index the index of the descriptor
 * @param data where to store the output string
 * @returns true if we got the value, false otherwise
 */
bool EuroliteProOutputPort::GetDescriptorString(
    libusb_device_handle *usb_handle,
    uint8_t desc_index,
    string *data) {
  enum { buffer_size = 32 };
  unsigned char buffer[buffer_size];
  int r = libusb_get_string_descriptor_ascii(
      usb_handle,
      desc_index,
      buffer,
      buffer_size);

  if (r <= 0)
    return false;
  data->assign(reinterpret_cast<char*>(buffer));
  return true;
}


/**
 * Find the interface with the endpoint we're after. Usually this is interface
 * 1 but we check them all just in case.
 */
bool EuroliteProOutputPort::LocateInterface() {
  struct libusb_config_descriptor *device_config;
  if (libusb_get_config_descriptor(m_usb_device, 0, &device_config) != 0) {
    OLA_WARN << "Failed to get device config descriptor";
    return false;
  }

  OLA_DEBUG << static_cast<int>(device_config->bNumInterfaces) <<
    " interfaces found";
  for (unsigned int i = 0; i < device_config->bNumInterfaces; i++) {
    const struct libusb_interface *interface = &device_config->interface[i];
    for (int j = 0; j < interface->num_altsetting; j++) {
      const struct libusb_interface_descriptor *iface_descriptor =
        &interface->altsetting[j];
      for (uint8_t k = 0; k < iface_descriptor->bNumEndpoints; k++) {
        const struct libusb_endpoint_descriptor *endpoint =
          &iface_descriptor->endpoint[k];
        OLA_DEBUG << "Interface " << i << ", altsetting " << j << ", endpoint "
          << static_cast<int>(k) << ", endpoint address 0x" << std::hex <<
          static_cast<int>(endpoint->bEndpointAddress);
        if (endpoint->bEndpointAddress == ENDPOINT) {
          OLA_INFO << "Using interface " << i;
          m_interface_number = i;
          libusb_free_config_descriptor(device_config);
          return true;
        }
      }
    }
  }
  libusb_free_config_descriptor(device_config);
  return false;
}
}  // usbdmx
}  // plugin
}  // ola
