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
 * AnymaWidget.h
 * The synchronous and asynchronous Anyma widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ANYMAWIDGET_H_
#define PLUGINS_USBDMX_ANYMAWIDGET_H_

#include <libusb.h>
#include "ola/base/Macro.h"
#include "ola/DmxBuffer.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

/**
 * @brief The interface for the Anyma Widgets
 */
class AnymaWidgetInterface {
 public:
  virtual ~AnymaWidgetInterface() {}

  virtual bool Init() = 0;

  virtual bool SendDMX(const DmxBuffer &buffer) = 0;

  static const char EXPECTED_MANUFACTURER[];
  static const char EXPECTED_PRODUCT[];
};

/**
 * @brief An Anyma widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousAnymaWidget: public AnymaWidgetInterface {
 public:
  explicit SynchronousAnymaWidget(libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  libusb_device* const m_usb_device;
  std::auto_ptr<class AnymaThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousAnymaWidget);
};

/**
 * @brief An Anyma widget that uses asynchronous libusb operations.
 */
class AsynchronousAnymaWidget : public AnymaWidgetInterface {
 public:
  explicit AsynchronousAnymaWidget(libusb_device *usb_device);
  ~AsynchronousAnymaWidget();

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
  uint8_t *m_control_setup_buffer;

  TransferState m_transfer_state;
  ola::thread::Mutex m_mutex;

  struct libusb_transfer *m_transfer;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousAnymaWidget);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_ANYMAWIDGET_H_
