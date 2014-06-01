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
 * ImgStageLineDevice.h
 * Interface for the ImgStageLine device
 * Copyright (C) 2014 Peter Newman
 */

#ifndef PLUGINS_USBDMX_IMGSTAGELINEDEVICE_H_
#define PLUGINS_USBDMX_IMGSTAGELINEDEVICE_H_

#include <libusb.h>
#include <string>
#include "plugins/usbdmx/UsbDevice.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/*
 * An ImgStageLine device
 */
class ImgStageLineDevice: public UsbDevice {
 public:
  ImgStageLineDevice(ola::AbstractPlugin *owner,
                     libusb_device *usb_device)
    : UsbDevice(owner, "ImgStageLine USB Device", usb_device) {
  }

  std::string DeviceId() const { return "dmx-1usb"; }

 protected:
  bool StartHook();
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_IMGSTAGELINEDEVICE_H_
