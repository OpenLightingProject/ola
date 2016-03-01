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
 * VellemanK8062.h
 * The synchronous and asynchronous Velleman widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_VELLEMANK8062_H_
#define PLUGINS_USBDMX_VELLEMANK8062_H_

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
 * @brief The interface for the Velleman Widgets
 */
class VellemanK8062: public SimpleWidget {
 public:
  explicit VellemanK8062(ola::usb::LibUsbAdaptor *adaptor,
                         libusb_device *usb_device)
      : SimpleWidget(adaptor, usb_device) {
  }
};

/**
 * @brief An Velleman widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousVellemanK8062: public VellemanK8062 {
 public:
  /**
   * @brief Create a new SynchronousVellemanK8062.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  SynchronousVellemanK8062(ola::usb::LibUsbAdaptor *adaptor,
                           libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class VellemanThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousVellemanK8062);
};

/**
 * @brief An Velleman widget that uses asynchronous libusb operations.
 */
class AsynchronousVellemanK8062 : public VellemanK8062 {
 public:
  /**
   * @brief Create a new AsynchronousVellemanK8062.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   */
  AsynchronousVellemanK8062(ola::usb::LibUsbAdaptor *adaptor,
                            libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class VellemanAsyncUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousVellemanK8062);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_VELLEMANK8062_H_
