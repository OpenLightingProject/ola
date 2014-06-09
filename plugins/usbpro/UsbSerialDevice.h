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
 * UsbSerialDevice.h
 * Interface for the usb devices
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBSERIALDEVICE_H_
#define PLUGINS_USBPRO_USBSERIALDEVICE_H_

#include <string>
#include "ola/Callback.h"
#include "olad/Device.h"
#include "plugins/usbpro/SerialWidgetInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * A USB device
 */
class UsbSerialDevice: public ola::Device {
 public:
    UsbSerialDevice(ola::AbstractPlugin *owner,
                    const std::string &name,
                    SerialWidgetInterface *widget):
      Device(owner, name),
      m_widget(widget) {}

    virtual ~UsbSerialDevice() {}

    void PrePortStop() {
      m_widget->Stop();
    }

    void SetOnRemove(ola::SingleUseCallback0<void> *on_close) {
      m_widget->GetDescriptor()->SetOnClose(on_close);
    }

    SerialWidgetInterface *GetWidget() const { return m_widget; }

 protected:
    SerialWidgetInterface *m_widget;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_USBSERIALDEVICE_H_
