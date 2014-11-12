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
 * SunliteWidget.h
 * The synchronous and asynchronous Sunlite widgets.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_USBDMX_SUNLITEWIDGET_H_
#define PLUGINS_USBDMX_SUNLITEWIDGET_H_

#include <libusb.h>
#include "ola/base/Macro.h"
#include "ola/DmxBuffer.h"
#include "plugins/usbdmx/ThreadedUsbSender.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class SunliteThreadedSender;

/**
 * @brief The interface for the Sunlite Widgets
 */
class SunliteWidget {
 public:
  virtual ~SunliteWidget() {}

  virtual bool Init() = 0;

  virtual bool SendDMX(const DmxBuffer &buffer) = 0;

  enum {SUNLITE_PACKET_SIZE = 0x340};
};


/**
 * @brief An Sunlite widget that uses synchronous libusb operations.
 *
 * Internally this spawns a new thread to avoid blocking SendDMX() calls.
 */
class SynchronousSunliteWidget: public SunliteWidget {
 public:
  explicit SynchronousSunliteWidget(libusb_device *usb_device);

  bool Init();

  bool SendDMX(const DmxBuffer &buffer);

 private:
  libusb_device* const m_usb_device;
  std::auto_ptr<class SunliteThreadedSender> m_sender;

  DISALLOW_COPY_AND_ASSIGN(SynchronousSunliteWidget);
};

/**
 * @brief An Sunlite widget that uses asynchronous libusb operations.
 */
class AsynchronousSunliteWidget: public SunliteWidget {
 public:
  explicit AsynchronousSunliteWidget(libusb_device *usb_device);
  ~AsynchronousSunliteWidget();

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

  uint8_t m_packet[SUNLITE_PACKET_SIZE];

  DISALLOW_COPY_AND_ASSIGN(AsynchronousSunliteWidget);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SUNLITEWIDGET_H_
