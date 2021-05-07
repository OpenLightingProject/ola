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
 * EurolitePro.h
 * The synchronous and asynchronous EurolitePro widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_EUROLITEPRO_H_
#define PLUGINS_USBDMX_EUROLITEPRO_H_

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

class EuroliteProThreadedSender;

/**
 * @brief The EurolitePro Widget.
 */
class EurolitePro : public SimpleWidget {
 public:
  /**
   * @brief Create a new EurolitePro.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   * @param is_mk2 whether the widget is a mk 2 variant.
   */
  EurolitePro(ola::usb::LibUsbAdaptor *adaptor,
              libusb_device *usb_device,
              const std::string &serial,
              bool is_mk2)
      : SimpleWidget(adaptor, usb_device),
        m_is_mk2(is_mk2),
        m_serial(serial) {}

  /**
   * @brief Get the serial number of this widget.
   * @returns The serial number of the widget.
   */
  std::string SerialNumber() const {
    return m_serial;
  }

 protected:
  bool m_is_mk2;

 private:
  std::string m_serial;
};


/**
 * @brief An EurolitePro widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousEurolitePro: public EurolitePro {
 public:
  /**
   * @brief Create a new SynchronousEurolitePro.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   * @param is_mk2 whether the widget is a mk 2 variant.
   */
  SynchronousEurolitePro(ola::usb::LibUsbAdaptor *adaptor,
                         libusb_device *usb_device,
                         const std::string &serial,
                         bool is_mk2);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class EuroliteProThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousEurolitePro);
};

/**
 * @brief An EurolitePro widget that uses asynchronous libusb operations.
 */
class AsynchronousEurolitePro: public EurolitePro {
 public:
  /**
   * @brief Create a new AsynchronousEurolitePro.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   * @param is_mk2 whether the widget is a mk 2 variant.
   */
  AsynchronousEurolitePro(ola::usb::LibUsbAdaptor *adaptor,
                          libusb_device *usb_device,
                          const std::string &serial,
                          bool is_mk2);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  std::auto_ptr<class EuroliteProAsyncUsbSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousEurolitePro);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_EUROLITEPRO_H_
