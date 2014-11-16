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
 * ScanlimeDevice.cpp
 * The Scanlime usb driver
 * Copyright (C) 2014 Peter Newman
 */

#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <algorithm>
#include <limits>
#include <string>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/util/Utils.h"
#include "plugins/usbdmx/ScanlimeDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

using std::string;

const char ScanlimeDevice::EXPECTED_MANUFACTURER[] = "scanlime";
const char ScanlimeDevice::EXPECTED_PRODUCT[] = "Fadecandy";


/**
 * New ScanlimeDevice.
 * @param owner the plugin that owns this device
 * @param usb_device a USB device
 * @param usb_handle a claimed handle to the device. Ownership is transferred.
 * @param serial the serial number, may be empty.
 */
ScanlimeDevice::ScanlimeDevice(ola::AbstractPlugin *owner,
                               libusb_device *usb_device,
                               libusb_device_handle *usb_handle,
                               const string &serial)
    : UsbDevice(owner, "Scanlime USB Device", usb_device),
      m_output_port(new ScanlimeOutputPort(this, 0, usb_handle)),
      m_serial(serial) {
  // Config
  uint8_t packet[64];
  memset(&packet, 0, sizeof(packet));
  packet[0] = 0x80;
  packet[1] |= (1 << 0);  // Disable dithering
  packet[1] |= (1 << 1);  // Disable interpolation
  //packet[1] = (1 << 2);  // Manual control of LED

  if (true) {
    //packet[1] |= (1 << 3);  // Manual LED state
  }

  int txed = 0;

  libusb_bulk_transfer(usb_handle,
                       1,
                       packet,
                       sizeof(packet),
                       &txed,
                       2000);

  OLA_INFO << "Config transferred " << txed << " bytes";

  // Look Up Table
  uint16_t lut[3][257];
  memset(&lut, 0, sizeof(lut));
  for (unsigned int channel = 0; channel < 3; channel++) {
    for (unsigned int value = 0; value < 257; value++) {
      // Fadecandy Python Example
      // lut[channel][value] = std::min(
      //     static_cast<int>(std::numeric_limits<uint16_t>::max()),
      //     int(pow(value / 256.0, 2.2) *
      //         (std::numeric_limits<uint16_t>::max() + 1)));
      // 1:1
      lut[channel][value] = std::min(
          static_cast<unsigned int>(std::numeric_limits<uint16_t>::max()),
          (value << 8));
      OLA_DEBUG << "Generated LUT for channel " << channel << " value "
                << value << " with val " << lut[channel][value];
    }
  }

  OLA_INFO << "LUT size " << (sizeof(lut) / 2);

  unsigned int index = 0;
  memset(&packet, 0, sizeof(packet));

  for (unsigned int channel = 0; channel < 3; channel++) {
    for (unsigned int value = 0; value < 257; value++) {
      unsigned int entry = (channel * 257) + value;
      unsigned int packet_entry = ((entry % 31) + 1);
      OLA_DEBUG << "Working on channel " << channel << " value " << value
                << " (" << IntToHexString(value) << ") with entry " << entry
                << ", packet entry " << packet_entry << " with val "
                << IntToHexString(lut[channel][value]);
      ola::utils::SplitUInt16(lut[channel][value],
                              &packet[packet_entry + 1],
                              &packet[packet_entry]);
      if ((packet_entry == 31) || (entry == ((3*257) - 1))) {
        if (entry == ((3*257) - 1)) {
          OLA_DEBUG << "Setting final flag on packet";
          packet[0] |= (1 << 5);  // Final
        }
        packet[0] |= 0x40;
        packet[0] |= index;
        packet[1] = 0;  // Reserved

        // Send the data
        int lut_txed = 0;

        // TODO(Peter): Fix the calculations and transmit this
        //libusb_bulk_transfer(usb_handle,
        //                     1,
        //                     packet,
        //                     sizeof(packet),
        //                     &lut_txed,
        //                     2000);

        OLA_INFO << "LUT packet " << index << " transferred " << lut_txed << " bytes";

        // Get ready for the next packet
        index++;
        memset(&packet, 0, sizeof(packet));
      }
    }
  }
}


/*
 * Start this device.
 */
bool ScanlimeDevice::StartHook() {
  if (!m_output_port->Start()) {
    delete m_output_port;
    m_output_port = NULL;
    return false;
  }
  AddPort(m_output_port);
  return true;
}


/*
 * Get the device id
 */
string ScanlimeDevice::DeviceId() const {
  if (!SerialNumber().empty()) {
    return "scanlime-" + SerialNumber();
  } else {
    return "";
  }
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
