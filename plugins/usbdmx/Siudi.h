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
 * Siudi.h
 * The synchronous SIUDI widgets.
 * Copyright (C) 2023 Alexander Simon
 */

#ifndef PLUGINS_USBDMX_SIUDI_H_
#define PLUGINS_USBDMX_SIUDI_H_

#include <libusb.h>
#include <memory>
#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"
#include "plugins/usbdmx/Widget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class SiudiThreadedSender;

/**
 * @brief The interface for the Siudi Widgets
 */
class Siudi : public SimpleWidget {
 public:
  explicit Siudi(ola::usb::LibUsbAdaptor *adaptor,
                   libusb_device *usb_device)
     : SimpleWidget(adaptor, usb_device) {
  }
};


/**
 * @brief An SIUDI widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousSiudi: public Siudi {
 public:
  /**
   * @brief Create a new SynchronousSiudi.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  SynchronousSiudi(ola::usb::LibUsbAdaptor *adaptor,
                     libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class ThreadedUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousSiudi);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SIUDI_H_
