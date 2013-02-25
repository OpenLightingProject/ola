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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * EuroliteProOutputPort.h
 * The output port for a EurolitePro device.
 * Copyright (C) 2011 Simon Newton & Harry F
 * Eurolite Pro USB DMX   ArtNo. 51860120
 */

#ifndef PLUGINS_USBDMX_EUROLITEPROOUTPUTPORT_H_
#define PLUGINS_USBDMX_EUROLITEPROOUTPUTPORT_H_

#include <libusb.h>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/thread/Thread.h"
#include "olad/Port.h"

namespace ola {
namespace plugin {
namespace usbdmx {


class EuroliteProOutputPort: public BasicOutputPort, ola::thread::Thread {
  public:
    EuroliteProOutputPort(class EuroliteProDevice *parent,
                          unsigned int id,
                          libusb_device *usb_device);
    ~EuroliteProOutputPort();
    string SerialNumber() const { return m_serial; }

    bool Start();
    void *Run();

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return ""; }

  private:
    static const unsigned int URB_TIMEOUT_MS = 500;
    static const unsigned int UDMX_SET_CHANNEL_RANGE = 0x0002;
    static const unsigned char ENDPOINT = 0x02;
    static const char EXPECTED_MANUFACTURER[];
    static const char EXPECTED_PRODUCT[];
    static const uint8_t DMX_LABEL = 6;

    bool m_term;
    int m_interface_number;
    string m_serial;

    libusb_device *m_usb_device;
    libusb_device_handle *m_usb_handle;
    DmxBuffer m_buffer;
    ola::thread::Mutex m_data_mutex;
    ola::thread::Mutex m_term_mutex;

    bool SendDMX(const DmxBuffer &buffer_old);

    bool GetDescriptorString(libusb_device_handle *usb_handle,
                             uint8_t desc_index,
                             string *data);
    bool LocateInterface();

    // 513 + header + code + size(2) + footer
    enum { FRAME_SIZE = 518 };
};
}  // usbdmx
}  // plugin
}  // ola
#endif  // PLUGINS_USBDMX_EUROLITEPROOUTPUTPORT_H_
