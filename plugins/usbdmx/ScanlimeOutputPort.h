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
 * ScanlimeOutputPort.h
 * The output port for a Scanlime device.
 * Copyright (C) 2014 Peter Newman
 *
 * It takes around ??? (TODO(Peter)) to send one universe of data. so we do this in a
 * separate thread.
 */

#ifndef PLUGINS_USBDMX_SCANLIMEOUTPUTPORT_H_
#define PLUGINS_USBDMX_SCANLIMEOUTPUTPORT_H_

#include <libusb.h>
#include <pthread.h>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"
#include "olad/Port.h"

namespace ola {
namespace plugin {
namespace usbdmx {

class ScanlimeDevice;

class ScanlimeOutputPort: public BasicOutputPort, ola::thread::Thread {
 public:
    ScanlimeOutputPort(ScanlimeDevice *parent,
                       unsigned int id,
                       libusb_device_handle *usb_handle);
    ~ScanlimeOutputPort();

    bool Start();
    void *Run();

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    std::string Description() const { return ""; }

 private:
    static const unsigned int URB_TIMEOUT_MS = 500;
    static const unsigned int UDMX_SET_CHANNEL_RANGE = 0x0002;

    bool m_term;
    std::string m_serial;
    libusb_device_handle *m_usb_handle;
    DmxBuffer m_buffer;
    ola::thread::Mutex m_data_mutex;
    ola::thread::Mutex m_term_mutex;

    bool SendDMX(const DmxBuffer &buffer_old);
};
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBDMX_SCANLIMEOUTPUTPORT_H_
