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
 * AnymaOutputPort.h
 * The output port for a Anyma device.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBDMX_ANYMAOUTPUTPORT_H_
#define PLUGINS_USBDMX_ANYMAOUTPUTPORT_H_

#include <libusb.h>
#include <pthread.h>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/OlaThread.h"
#include "olad/Port.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class AnymaDevice;

class AnymaOutputPort: public BasicOutputPort, OlaThread {
  public:
    AnymaOutputPort(AnymaDevice *parent,
                    unsigned int id,
                    libusb_device *usb_device);
    ~AnymaOutputPort();

    bool Start();
    void *Run();

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return ""; }

  private:
    static const unsigned int URB_TIMEOUT_MS = 500;
    static const unsigned int UDMX_SET_CHANNEL_RANGE = 0x0002;

    bool m_term;
    libusb_device *m_usb_device;
    libusb_device_handle *m_usb_handle;
    DmxBuffer m_buffer;
    pthread_mutex_t m_data_mutex;
    pthread_mutex_t m_term_mutex;

    bool SendDMX(const DmxBuffer &buffer_old);
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_ANYMAOUTPUTPORT_H_
