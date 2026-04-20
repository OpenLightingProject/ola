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
 * Widget.h
 * A generic USB Widget.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_WIDGET_H_
#define PLUGINS_USBDMX_WIDGET_H_

#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/Types.h"
#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief The interface for a simple widget that supports a single universe of
 * DMX.
 */
class WidgetInterface {
 public:
  virtual ~WidgetInterface() {}

  /**
   * @brief Initialize the widget.
   * @returns true if this widget was initialized correctly, false otherwise.
   */
  virtual bool Init() = 0;

  /**
   * @brief The device ID of this widget.
   * @returns The USBDeviceID.
   */
  virtual ola::usb::USBDeviceID GetDeviceId() const = 0;

  /**
   * @brief Send DMX data from this widget
   * @param buffer The DmxBuffer containing the data to send.
   * @returns true if the data was sent, false otherwise.
   */
  virtual bool SendDMX(const DmxBuffer &buffer, unsigned int portId = 0) = 0;
};

/**
 * @brief A base simple widget class.
 *
 * This holds a pointer to a LibUsbAdaptor so we don't have to duplicate that
 * code in all the Widgets.
 */
class SimpleWidget : public WidgetInterface {
 public:
  /**
   * @brief Create a new SimpleWidget.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device The libusb_device for this widget.
   */
  explicit SimpleWidget(ola::usb::LibUsbAdaptor *adaptor,
                        libusb_device *usb_device)
     : m_adaptor(adaptor),
       m_usb_device(usb_device) {
  }

  ola::usb::USBDeviceID GetDeviceId() const {
    return m_adaptor->GetDeviceId(m_usb_device);
  }

 protected:
  ola::usb::LibUsbAdaptor* const m_adaptor;
  libusb_device* const m_usb_device;
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_WIDGET_H_
