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
#include "ola/base/Macro.h"
#include "ola/DmxBuffer.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class EuroliteProThreadedSender;

/**
 * @brief The interface for the EurolitePro Widgets
 */
class EuroliteProWidgetInterface {
 public:
  virtual ~EuroliteProWidgetInterface() {}

  virtual bool Init() = 0;

  virtual bool SendDMX(const DmxBuffer &buffer) = 0;

  static const char EXPECTED_MANUFACTURER[];
  static const char EXPECTED_PRODUCT[];

  // 513 + header + code + size(2) + footer
  enum { EUROLITE_PRO_FRAME_SIZE = 518 };
};


/**
 * @brief An EurolitePro widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousEuroliteProWidget: public EuroliteProWidgetInterface {
 public:
  explicit SynchronousEuroliteProWidget(libusb_device *usb_device);

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
class AsynchronousEuroliteProWidget: public EuroliteProWidgetInterface {
 public:
  explicit AsynchronousEuroliteProWidget(libusb_device *usb_device);
  ~AsynchronousEuroliteProWidget();

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

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
