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
 * VellemanOutputPort.h
 * The output port for a Velleman 8062 device.
 * Copyright (C) 2010 Simon Newton
 *
 * Because this interface is so slow we run the output in a separate thread. It
 * takes around 8ms to respond to an urb and in the worst case we send 74 urbs
 * per universe.
 *
 * It would be interesting to see if you can pipeline the urbs to improve the
 * performance.
 */

#ifndef PLUGINS_USBDMX_VELLEMANOUTPUTPORT_H_
#define PLUGINS_USBDMX_VELLEMANOUTPUTPORT_H_

#include <libusb.h>
#include <pthread.h>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/OlaThread.h"
#include "olad/Port.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class VellemanDevice;

class VellemanOutputPort: public BasicOutputPort, OlaThread {
  public:
    VellemanOutputPort(VellemanDevice *parent,
                       unsigned int id,
                       libusb_device *usb_device);
    ~VellemanOutputPort();

    bool Start();
    void *Run();

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const;

  private:
    static const unsigned char ENDPOINT = 0x01;
    // 25ms seems to be about the shortest we can go
    static const unsigned int URB_TIMEOUT_MS = 25;
    static const int CONFIGURATION = 1;
    static const int INTERFACE = 0;
    static const int UPGRADED_CHUNK_SIZE = 64;

    bool m_term;
    unsigned int m_chunk_size;
    libusb_device *m_usb_device;
    libusb_device_handle *m_usb_handle;
    DmxBuffer m_buffer;
    ola::Mutex m_data_mutex;
    ola::Mutex m_term_mutex;

    bool SendDMX(const DmxBuffer &buffer_old);
    bool SendDataChunk(uint8_t *usb_data);
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_VELLEMANOUTPUTPORT_H_
