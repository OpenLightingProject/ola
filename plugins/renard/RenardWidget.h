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
 * RenardWidget.h
 * Interface for the renard widget
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDWIDGET_H_
#define PLUGINS_RENARD_RENARDWIDGET_H_

#include <fcntl.h>
#include <termios.h>
#include <string>

#include "ola/io/SelectServer.h"
#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace renard {

class RenardWidget {
 public:
    // The DMX offset is where in the DMX universe the Renard channels
    // will be mapped. Set to 0 means the first Renard channel will be
    // mapped to DMX channel 1, next to 2, etc. If you set the DMX offset
    // to 100 then the first Renard channel will respond to DMX channel
    // 101. I envisioned this would be applicable to multiple (serial)
    // devices sharing the same DMX universe.
    // Number of channels is how many channels we'll output on the device.
    // There are limits to how many channels we can address for any given
    // refresh rate, based on baud rate and escaping. Renard ignores
    // any extra channels that are sent on the wire, so setting this too
    // high is not a major concern.
    // The start address is the Renard-address of the first board. The
    // default in the standard firmware is 0x80, and it may be a reasonable
    // future feature request to have this configurable for more advanced
    // Renard configurations (using wireless transmitters, etc).
    explicit RenardWidget(const std::string &path,
                          int dmxOffset,
                          int channels,
                          uint32_t baudrate,
                          uint8_t startAddress)
      : m_path(path),
        m_socket(NULL),
        m_byteCounter(0),
        m_dmxOffset(dmxOffset),
        m_channels(channels),
        m_baudrate(baudrate),
        m_startAddress(startAddress) {}
    virtual ~RenardWidget();

    // these methods are for communicating with the device
    bool Connect();
    int Disconnect();
    ola::io::ConnectedDescriptor *GetSocket() { return m_socket; }
    string GetPath() { return m_path; }
    bool SendDmx(const DmxBuffer &buffer);
    bool DetectDevice();

    static const uint8_t RENARD_CHANNELS_IN_BANK;

 private:
    int ConnectToWidget(const std::string &path, speed_t speed);

    // instance variables
    const string m_path;
    ola::io::ConnectedDescriptor *m_socket;
    uint32_t m_byteCounter;
    uint32_t m_dmxOffset;
    uint32_t m_channels;
    uint32_t m_baudrate;
    uint8_t m_startAddress;

    static const uint8_t RENARD_COMMAND_PAD;
    static const uint8_t RENARD_COMMAND_START_PACKET;
    static const uint8_t RENARD_COMMAND_ESCAPE;
    static const uint8_t RENARD_ESCAPE_PAD;
    static const uint8_t RENARD_ESCAPE_START_PACKET;
    static const uint8_t RENARD_ESCAPE_ESCAPE;
    static const uint32_t RENARD_BYTES_BETWEEN_PADDING;
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDWIDGET_H_
