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
 * ShowJockeyDMXU1.h
 * The synchronous and asynchronous ShowJockeyDMXU1 widgets.

 * Adapted from SJ-DMX, p3root - Patrik Pfaffenbauer,
 * patrik.pfaffenbauer@p3.co.at,
 * https://github.com/p3root/SJ-DMX.git
 *
 * by Nicolas Bertrand, nbe@anomes.com
 *
 * Copyright (C) 2016 Nicolas Bertrand
 */

#ifndef PLUGINS_USBDMX_SHOWJOCKEYDMXU1_H_
#define PLUGINS_USBDMX_SHOWJOCKEYDMXU1_H_

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

class ShowJockeyDMXU1ThreadedSender;

/**
 * @brief The ShowJockey-DMX-U1 Widget.
 *
 * Stream value to the ShowJockey-DMX-U1 by respecting this packet format:
 * The first two bytes describe on a uint16_t the index of the first channel from which the
 * data starts. Each following bytes correspond to the value of one channel.
 */
class ShowJockeyDMXU1 : public SimpleWidget {
 public:
  /**
   * @brief Create a new ShowJockeyDMXU1.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  ShowJockeyDMXU1(ola::usb::LibUsbAdaptor *adaptor,
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
 * @brief A ShowJockeyDMXU1 widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousShowJockeyDMXU1: public ShowJockeyDMXU1 {
 public:
  /**
   * @brief Create a new SynchronousShowJockeyDMXU1.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  SynchronousShowJockeyDMXU1(ola::usb::LibUsbAdaptor *adaptor,
                             libusb_device *usb_device,
                             const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class ShowJockeyDMXU1ThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousShowJockeyDMXU1);
};

/**
 * @brief A ShowJockeyDMXU1 widget that uses asynchronous libusb operations.
 */
class AsynchronousShowJockeyDMXU1: public ShowJockeyDMXU1 {
 public:
  /**
   * @brief Create a new AsynchronousShowJockeyDMXU1.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  AsynchronousShowJockeyDMXU1(ola::usb::LibUsbAdaptor *adaptor,
                              libusb_device *usb_device,
                              const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class ShowJockeyDMXU1AsyncUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousShowJockeyDMXU1);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SHOWJOCKEYDMXU1_H_
