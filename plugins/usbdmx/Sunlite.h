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
 * Sunlite.h
 * The synchronous and asynchronous Sunlite widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_SUNLITE_H_
#define PLUGINS_USBDMX_SUNLITE_H_

#include <libusb.h>
#include <memory>
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"
#include "plugins/usbdmx/Widget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class SunliteThreadedSender;

/**
 * @brief The interface for the Sunlite Widgets
 */
class Sunlite : public SimpleWidget {
 public:
  explicit Sunlite(ola::usb::LibUsbAdaptor *adaptor,
                   libusb_device *usb_device)
     : SimpleWidget(adaptor, usb_device) {
  }
};


/**
 * @brief An Sunlite widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousSunlite: public Sunlite {
 public:
  /**
   * @brief Create a new SynchronousSunlite.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  SynchronousSunlite(ola::usb::LibUsbAdaptor *adaptor,
                     libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class SunliteThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousSunlite);
};

/**
 * @brief An Sunlite widget that uses asynchronous libusb operations.
 */
class AsynchronousSunlite: public Sunlite {
 public:
  /**
   * @brief Create a new AsynchronousSunlite.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  AsynchronousSunlite(ola::usb::LibUsbAdaptor *adaptor,
                      libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class SunliteAsyncUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousSunlite);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SUNLITE_H_
