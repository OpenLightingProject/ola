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
 * ImgStageLineOutputPort.h
 * The output port for an img Stage Line DMX-1USB device.
 * Copyright (C) 2014 Peter Newman
 *
 * It takes around 11ms to complete the transfer to the device so we use a
 * a separate thread for the writes. The time to acquire the lock, copy the
 * buffer & release is 1-2 uS.
 */

#ifndef PLUGINS_USBDMX_IMGSTAGELINEOUTPUTPORT_H_
#define PLUGINS_USBDMX_IMGSTAGELINEOUTPUTPORT_H_

#include <libusb.h>
#include <pthread.h>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"
#include "olad/Port.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class ImgStageLineDevice;

class ImgStageLineOutputPort: public BasicOutputPort, ola::thread::Thread {
 public:
  ImgStageLineOutputPort(ImgStageLineDevice *parent,
                         unsigned int id,
                         libusb_device *usb_device);
  ~ImgStageLineOutputPort();

  bool Start();
  void *Run();

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
  std::string Description() const { return ""; }

 private:
  static const unsigned int CHANNELS_PER_PACKET = 255;
  static const uint8_t CHANNEL_HEADER_LOW = 0x7f;
  static const uint8_t CHANNEL_HEADER_HIGH = 0xff;
  static const uint8_t ENDPOINT = 1;
  static const unsigned int TIMEOUT = 50;  // 50ms is ok

  enum {IMGSTAGELINE_PACKET_SIZE = CHANNELS_PER_PACKET + 1};

  // This interface can only transmit 510 channels
  enum { DMX_MAX_TRANSMIT_CHANNELS = 510 };

  bool m_term;
  bool m_new_data;
  uint8_t m_packet[IMGSTAGELINE_PACKET_SIZE];
  libusb_device *m_usb_device;
  libusb_device_handle *m_usb_handle;
  DmxBuffer m_buffer;
  ola::thread::Mutex m_data_mutex;
  ola::thread::Mutex m_term_mutex;
  int m_packet_count;

  bool SendDMX(const DmxBuffer &buffer);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_IMGSTAGELINEOUTPUTPORT_H_
