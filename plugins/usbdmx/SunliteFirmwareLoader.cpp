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
 * SunliteFirmwareLoader.cpp
 * Load the firmware onto a USBDMX2 device.
 * Copyright (C) 2010 Simon Newton
 */

#include "ola/Logging.h"
#include "plugins/usbdmx/SunliteFirmware.h"
#include "plugins/usbdmx/SunliteFirmwareLoader.h"


namespace ola {
namespace plugin {
namespace usbdmx {


/*
 * Load the firmware
 */
bool SunliteFirmwareLoader::LoadFirmware() {
  libusb_device_handle *handle;

  if (libusb_open(m_device, &handle)) {
    OLA_WARN << "Failed to open sunlite device";
    return false;
  }

  if (libusb_claim_interface(handle, INTERFACE_NUMBER)) {
    OLA_WARN << "Failed to claim sunlite device.";
    libusb_close(handle);
    return false;
  }

  const struct sunlite_hex_record *record = sunlite_firmware;

  while (record->address != SUNLITE_END_OF_FIRMWARE) {
    int ret = libusb_control_transfer(
        handle,
        UPLOAD_REQUEST_TYPE,
        UPLOAD_REQUEST,
        record->address,
        0,
        (unsigned char*) record->data,
        record->data_size,
        UPLOAD_TIMEOUT);
    if (ret != record->data_size) {
      OLA_WARN << "Sunlite firmware load failed, address: " << record->address
        << ", ret value was " << ret;
      libusb_release_interface(handle, INTERFACE_NUMBER);
      libusb_close(handle);
    }
    record++;
  }

  libusb_release_interface(handle, INTERFACE_NUMBER);
  libusb_close(handle);
  return true;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
