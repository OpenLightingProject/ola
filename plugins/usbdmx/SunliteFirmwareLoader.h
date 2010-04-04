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
 * SunliteFirmwareLoader.h
 * Load the firmware onto a USBDMX2 device
 * Copyright (C) 2010 Simon Newton
 *
 * The Soundlite USBDMX2 interfaces require firmware to be loaded when they're
 * connected, this performs that job.
 */

#ifndef PLUGINS_USBDMX_SUNLITEFIRMWARELOADER_H_
#define PLUGINS_USBDMX_SUNLITEFIRMWARELOADER_H_

#include <libusb.h>
#include "plugins/usbdmx/FirmwareLoader.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class SunliteFirmwareLoader: public FirmwareLoader {
  public:
    explicit SunliteFirmwareLoader(libusb_device *usb_device)
        : m_device(usb_device) {}
    ~SunliteFirmwareLoader() {}

    bool LoadFirmware();

  private:
    libusb_device *m_device;

    static const int INTERFACE_NUMBER = 0;  // the device only has 1 interface
    static const uint8_t UPLOAD_REQUEST_TYPE = 0x40;
    static const uint8_t UPLOAD_REQUEST = 0xa0;
    static const unsigned int UPLOAD_TIMEOUT = 300;  // ms
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_SUNLITEFIRMWARELOADER_H_
