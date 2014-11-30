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
 * AnymauDMX.h
 * The synchronous and asynchronous Anyma uDMX widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ANYMAUDMX_H_
#define PLUGINS_USBDMX_ANYMAUDMX_H_

#include <libusb.h>
#include <memory>
#include <string>

#include "ola/DmxBuffer.h"
#include "ola/base/Macro.h"
#include "ola/thread/Mutex.h"
#include "plugins/usbdmx/Widget.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief The base class for Anyma Widgets.
 */
class AnymauDMX: public BaseWidget {
 public:
  /**
   * @brief Create a new AnymauDMX.
   * @param adaptor the LibUsbAdaptor to use.
   * @param serial the serial number of the widget.
   */
  AnymauDMX(LibUsbAdaptor *adaptor,
            const std::string &serial)
      : BaseWidget(adaptor),
        m_serial(serial) {}

  virtual ~AnymauDMX() {}

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
 * @brief An Anyma widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousAnymauDMX: public AnymauDMX {
 public:
  /**
   * @brief Create a new SynchronousAnymauDMX.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  SynchronousAnymauDMX(LibUsbAdaptor *adaptor,
                       libusb_device *usb_device,
                       const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  libusb_device* const m_usb_device;
  std::auto_ptr<class AnymaThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousAnymauDMX);
};

/**
 * @brief An Anyma widget that uses asynchronous libusb operations.
 */
class AsynchronousAnymauDMX : public AnymauDMX {
 public:
  /**
   * @brief Create a new AsynchronousAnymauDMX.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  AsynchronousAnymauDMX(LibUsbAdaptor *adaptor,
                        libusb_device *usb_device,
                        const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class AnymaAsyncUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousAnymauDMX);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ANYMAUDMX_H_
