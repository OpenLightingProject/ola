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
 * ShowJockey.h
 * The synchronous and asynchronous ShowJockey widgets.
 * Copyright (C) 2016 Nicolas Bertrand
 */

#ifndef PLUGINS_USBDMX_SHOWJOCKEY_H_
#define PLUGINS_USBDMX_SHOWJOCKEY_H_

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

class ShowJockeyThreadedSender;

/**
 * @brief The ShowJockey Widget.
 *
 * Stream value to the ShowJockey by respecting this packet format:
 * The first two bytes describe on a uint16_t the index of the first canal from which the
 * data start. Each following bytes correspond to the value of one canal. 
 *
 */
class ShowJockey : public SimpleWidget {
 public:
  /**
   * @brief Create a new ShowJockey.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  ShowJockey(ola::usb::LibUsbAdaptor *adaptor,
             libusb_device *usb_device,
             const std::string &serial)
      : SimpleWidget(adaptor, usb_device),
        m_serial(serial) {}

  /**
   * @brief Get the serial number of this widget.
   * @returns The serial number of the widget.
   */
  std::string SerialNumber() const {
    return m_serial;
  }

 protected:
  std::string m_serial;
};


/**
 * @brief An ShowJockey widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousShowJockey: public ShowJockey {
 public:
  /**
   * @brief Create a new SynchronousShowJockey.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  SynchronousShowJockey(ola::usb::LibUsbAdaptor *adaptor,
                        libusb_device *usb_device,
                        const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class ShowJockeyThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousShowJockey);
};

/**
 * @brief An ShowJockey widget that uses asynchronous libusb operations.
 */
class AsynchronousShowJockey: public ShowJockey {
 public:
  /**
   * @brief Create a new AsynchronousShowJockey.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  AsynchronousShowJockey(ola::usb::LibUsbAdaptor *adaptor,
                         libusb_device *usb_device,
                         const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class ShowJockeyAsyncUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousShowJockey);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SHOWJOCKEY_H_
