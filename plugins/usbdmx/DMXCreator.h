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
 * DMXCreator.h
 * The synchronous and asynchronous DMXCreator widgets.
 * Copyright (C) 2016 Florian Edelmann
 */

#ifndef PLUGINS_USBDMX_DMXCREATOR_H_
#define PLUGINS_USBDMX_DMXCREATOR_H_

#include <libusb.h>
#include <memory>
#include <string>

#include "libs/usb/LibUsbAdaptor.h"
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"
#include "plugins/usbdmx/Widget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief The base class for DMXCreator Widgets.
 */
class DMXCreator: public SimpleWidget {
 public:
  /**
   * @brief Create a new DMXCreator.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  DMXCreator(ola::usb::LibUsbAdaptor *adaptor,
             libusb_device *usb_device,
             const std::string &serial)
      : SimpleWidget(adaptor, usb_device),
        m_serial(serial) {}

  virtual ~DMXCreator() {}

  /**
   * @brief Get the serial number of this widget.
   * @returns The serial number of the widget.
   */
  std::string SerialNumber() const {
    return m_serial;
  }

 private:
  std::string m_serial;
};

/**
 * @brief An DMXCreator widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousDMXCreator: public DMXCreator {
 public:
  /**
   * @brief Create a new SynchronousDMXCreator.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  SynchronousDMXCreator(ola::usb::LibUsbAdaptor *adaptor,
                        libusb_device *usb_device,
                        const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class DMXCreatorThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousDMXCreator);
};

/**
 * @brief An DMXCreator widget that uses asynchronous libusb operations.
 */
class AsynchronousDMXCreator : public DMXCreator {
 public:
  /**
   * @brief Create a new AsynchronousDMXCreator.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  AsynchronousDMXCreator(ola::usb::LibUsbAdaptor *adaptor,
                         libusb_device *usb_device,
                         const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class DMXCreatorAsyncUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousDMXCreator);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_DMXCREATOR_H_
