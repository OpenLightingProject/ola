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
 * VellemanWidget.h
 * The synchronous and asynchronous Velleman widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_VELLEMANWIDGET_H_
#define PLUGINS_USBDMX_VELLEMANWIDGET_H_

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
 * @brief The interface for the Velleman Widgets
 */
class VellemanWidget: public Widget {};

/**
 * @brief An Velleman widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousVellemanWidget: public VellemanWidget {
 public:
  /**
   * @brief Create a new SynchronousVellemanWidget.
   * @param usb_device the libusb_device to use for the widget.
   */
  explicit SynchronousVellemanWidget(libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  libusb_device* const m_usb_device;
  std::auto_ptr<class VellemanThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousVellemanWidget);
};

/**
 * @brief An Velleman widget that uses asynchronous libusb operations.
 */
class AsynchronousVellemanWidget : public VellemanWidget {
 public:
  /**
   * @brief Create a new AsynchronousVellemanWidget.
   * @param usb_device the libusb_device to use for the widget.
   */
  explicit AsynchronousVellemanWidget(libusb_device *usb_device);
  ~AsynchronousVellemanWidget();

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
  uint8_t *m_control_setup_buffer;

  TransferState m_transfer_state;
  ola::thread::Mutex m_mutex;

  struct libusb_transfer *m_transfer;

  DISALLOW_COPY_AND_ASSIGN(AsynchronousVellemanWidget);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_VELLEMANWIDGET_H_
