/*
 *  This dmxgram is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This dmxgram is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this dmxgram; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UsbDmxDevice.cpp
 * UsbDmx device
 * Copyright (C) 2006-2007 Simon Newton
 *
 * The device creates two ports, one in and one out, but you can only use one
 * at a time.
 */

#include <sys/time.h>
#include <string>

#include "ola/Closure.h"
#include "ola/Logging.h"
#include "olad/Preferences.h"
#include "plugins/usbdmx/UsbDmxDevice.h"
#include "plugins/usbdmx/UsbDmxPort.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * Create a new device
 * @param owner the plugin that owns this device
 * @param usb_device the libusb device
 */
UsbDmxDevice::UsbDmxDevice(ola::AbstractPlugin *owner,
                           libusb_device *usb_device):
  Device(owner, "Velleman USB Device"),
  m_enabled(false),
  m_usb_device(usb_device),
  m_usb_handle(NULL) {
}


/*
 * Destroy this device
 */
UsbDmxDevice::~UsbDmxDevice() {
  if (m_enabled)
    Stop();
}


/*
 * Start this device.
 */
bool UsbDmxDevice::Start() {
  if (libusb_open(m_usb_device, &m_usb_handle)) {
    OLA_WARN << "Failed to open usb device";
    return false;
  }

  if (libusb_kernel_driver_active(m_usb_handle, 0)) {
    if (libusb_detach_kernel_driver(m_usb_handle, 0)) {
      OLA_WARN << "Failed to detach kernel driver";
      libusb_close(m_usb_handle);
      return false;
    }
  }

  if (libusb_claim_interface(m_usb_handle, 0)) {
    OLA_WARN << "Failed to claim usb device";
    libusb_close(m_usb_handle);
    return false;
  }

  OutputPort *output_port = new UsbDmxOutputPort(this, 0);
  AddPort(output_port);
  m_enabled = true;
  return true;
}


/*
 * Stop this device
 */
bool UsbDmxDevice::Stop() {
  if (!m_enabled)
    return true;

  DeleteAllPorts();

  if (m_usb_handle) {
    libusb_release_interface(m_usb_handle, 0);
    libusb_close(m_usb_handle);
  }
  libusb_unref_device(m_usb_device);

  m_enabled = false;
  return true;
}


/*
 * Send the dmx out the widget
 * @return true on success, false on failure
 */
bool UsbDmxDevice::SendDMX(const DmxBuffer &buffer) {
  unsigned char usb_data[VELLEMAN_USB_CHUNK_SIZE];
  unsigned int i = 0;
  unsigned int size = buffer.Size();
  const uint8_t *data = buffer.GetRaw();

  memset(usb_data, 0, sizeof(usb_data));

  unsigned int n;
  for (n = 0; n < 100 && n < size - COMPRESSED_CHANNEL_COUNT && !data[n]; n++);
  OLA_INFO << "found " << n << " null channels at the start";
  usb_data[0] = 4;
  usb_data[1] = n + 1; // include start code
  memcpy(usb_data + 2, data, COMPRESSED_CHANNEL_COUNT);
  SendData(usb_data);
  i += COMPRESSED_CHANNEL_COUNT;

  while (i < size - 7) {
    for (n = 0; n < 100 && n + i < size - COMPRESSED_CHANNEL_COUNT && !data[i + n]; n++);
    OLA_INFO << "i: " << i << ", n: " << n;
    if (n) {
      // we have leading zeros
      usb_data[0] = 5;
      usb_data[1] = n;
      memcpy(usb_data + 2, data + i + n, COMPRESSED_CHANNEL_COUNT);
      i += COMPRESSED_CHANNEL_COUNT + n;
    } else {
      usb_data[0] = 5;
      memcpy(usb_data + 1, data + i, CHANNEL_COUNT);
      SendData(usb_data);
      i += CHANNEL_COUNT;
    }
    SendData(usb_data);
  }

  // send the last channels
  for (;i != size; i++) {
    usb_data[0] = 3;
    usb_data[1] = data[i];
    SendData(usb_data);
  }

  return true;
}


/*
 * Send 8 bytes to the usb device
 */
void UsbDmxDevice::SendData(uint8_t *usb_data) {
  unsigned char endpoint = 0x01;
  int transferred;
  unsigned int timeout = 50;

  OLA_INFO << "Sending " <<
    (int) usb_data[0] << "," <<
    (int) usb_data[1] << "," <<
    (int) usb_data[2] << "," <<
    (int) usb_data[3] << "," <<
    (int) usb_data[4] << "," <<
    (int) usb_data[5] << "," <<
    (int) usb_data[6] << "," <<
    (int) usb_data[7] << ",";
  int ret = libusb_bulk_transfer(m_usb_handle,
                                 endpoint,
                                 reinterpret_cast<unsigned char*>(usb_data),
                                 VELLEMAN_USB_CHUNK_SIZE,
                                 &transferred,
                                 timeout);
  OLA_INFO << "usb return code was " << ret;

}
}  // usbdmx
}  // plugin
}  // ola
