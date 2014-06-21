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
 * stageprofidevice.h
 * Interface for the stageprofi device
 * Copyright (C) 2006 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIWIDGET_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIWIDGET_H_

#include <string>
#include "ola/io/SelectServer.h"
#include "ola/network/Socket.h"
#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace stageprofi {


class StageProfiWidget {
 public:
    StageProfiWidget():
      m_enabled(false),
      m_got_response(false),
      m_socket(NULL),
      m_ss(NULL) {}
    virtual ~StageProfiWidget();

    // these methods are for communicating with the device
    virtual bool Connect(const std::string &path) = 0;
    int Disconnect();
    std::string GetDevicePath() { return m_device_path; }
    ola::io::ConnectedDescriptor *GetSocket() { return m_socket; }
    bool SendDmx(const DmxBuffer &buffer) const;
    bool DetectDevice();
    void SocketReady();
    void Timeout();

 protected:
    int Send255(uint16_t start, const uint8_t *buf, unsigned int len) const;
    int SetChannel(uint16_t chan, uint8_t val) const;

    // instance variables
    bool m_enabled;
    bool m_got_response;
    std::string m_device_path;
    ola::io::ConnectedDescriptor *m_socket;
    ola::io::SelectServer *m_ss;

    enum { DMX_MSG_LEN = 255 };
    enum { DMX_HEADER_SIZE = 4};

 private:
    int DoRecv();
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIWIDGET_H_
