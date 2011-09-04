/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SunliteOutputPort.h
 * The output port for a Sunlite USBDMX2 device.
 * Copyright (C) 2010 Simon Newton
 *
 * It takes around 11ms to complete the transfer to the device so we use a
 * a separate thread for the writes. The time to acquire the lock, copy the
 * buffer & release is 1-2 uS.
 */

#ifndef PLUGINS_USBDMX_SUNLITEOUTPUTPORT_H_
#define PLUGINS_USBDMX_SUNLITEOUTPUTPORT_H_

#include <libusb.h>
#include <pthread.h>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/OlaThread.h"
#include "olad/Port.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class SunliteDevice;

class SunliteOutputPort: public BasicOutputPort, OlaThread {
  public:
    SunliteOutputPort(SunliteDevice *parent,
                      unsigned int id,
                      libusb_device *usb_device);
    ~SunliteOutputPort();

    bool Start();
    void *Run();

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return ""; }

  private:
    enum {SUNLITE_PACKET_SIZE = 0x340};

    static const unsigned int CHUNKS_PER_PACKET = 26;
    static const unsigned int CHANNELS_PER_CHUNK = 20;
    static const unsigned int CHUNK_SIZE = 32;
    static const uint8_t ENDPOINT = 1;
    static const unsigned int TIMEOUT = 50;  // 50ms is ok

    bool m_term;
    bool m_new_data;
    uint8_t m_packet[SUNLITE_PACKET_SIZE];
    libusb_device *m_usb_device;
    libusb_device_handle *m_usb_handle;
    DmxBuffer m_buffer;
    ola::Mutex m_data_mutex;
    ola::Mutex m_term_mutex;

    void InitPacket();
    bool SendDMX(const DmxBuffer &buffer);
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_SUNLITEOUTPUTPORT_H_
