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
 * EuroliteProWidget.h
 * The synchronous and asynchronous EurolitePro widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_EUROLITEPROWIDGET_H_
#define PLUGINS_USBDMX_EUROLITEPROWIDGET_H_

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

class EuroliteProThreadedSender;

/**
 * @brief The EurolitePro Widget.
 */
class EuroliteProWidget : public BaseWidget {
 public:
  /**
   * @brief Create a new EuroliteProWidget.
   * @param adaptor the LibUsbAdaptor to use.
   * @param serial the serial number of the widget.
   */
  EuroliteProWidget(LibUsbAdaptor *adaptor,
                    const std::string &serial)
      : BaseWidget(adaptor),
        m_serial(serial) {}

  /**
   * @brief Get the serial number of this widget.
   * @returns The serial number of the widget.
   */
  std::string SerialNumber() const {
    return m_serial;
  }

  /**
   * @brief The size of a EurolitePro frame.
   *
   * This consists of 513 bytes of DMX data + header + code + size(2) + footer
   */
  enum { EUROLITE_PRO_FRAME_SIZE = 518 };

 private:
  std::string m_serial;
};


/**
 * @brief An EurolitePro widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousEuroliteProWidget: public EuroliteProWidget {
 public:
  /**
   * @brief Create a new SynchronousEuroliteProWidget.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  SynchronousEuroliteProWidget(LibUsbAdaptor *adaptor,
                               libusb_device *usb_device,
                               const std::string &serial);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  libusb_device* const m_usb_device;
  std::auto_ptr<class EuroliteProThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousEuroliteProWidget);
};

/**
 * @brief An EurolitePro widget that uses asynchronous libusb operations.
 */
class AsynchronousEuroliteProWidget: public EuroliteProWidget {
 public:
  /**
   * @brief Create a new AsynchronousEuroliteProWidget.
   * @param adaptor the LibUsbAdaptor to use.
   * @param usb_device the libusb_device to use for the widget.
   * @param serial the serial number of the widget.
   */
  AsynchronousEuroliteProWidget(class LibUsbAdaptor *adaptor,
                                libusb_device *usb_device,
                                const std::string &serial);
  ~AsynchronousEuroliteProWidget();

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

  /**
   * @brief Called from the libusb callback when the asynchronous transfer
   *   completes.
   * @param transfer the completed transfer.
   */
  void TransferComplete(struct libusb_transfer *transfer);

 private:
  enum TransferState {
    IDLE,
    IN_PROGRESS,
  };

  libusb_device* const m_usb_device;
  libusb_device_handle *m_usb_handle;

  TransferState m_transfer_state;
  ola::thread::Mutex m_mutex;

  struct libusb_transfer *m_transfer;

  uint8_t m_tx_frame[EUROLITE_PRO_FRAME_SIZE];

  DISALLOW_COPY_AND_ASSIGN(AsynchronousEuroliteProWidget);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_EUROLITEPROWIDGET_H_
