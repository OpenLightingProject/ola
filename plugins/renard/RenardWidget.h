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
    static int ConnectToWidget(const std::string &path, speed_t speed);

    explicit RenardWidget(const std::string &path):
      m_enabled(false),
      m_path(path),
      m_socket(NULL),
      m_ss(NULL) {}
    virtual ~RenardWidget();

    // these methods are for communicating with the device
    virtual bool Connect() = 0;
    int Disconnect();
    ola::io::ConnectedDescriptor *GetSocket() { return m_socket; }
    string GetPath() { return m_path; }
    virtual bool SendDmx(const DmxBuffer &buffer) = 0;
    virtual bool DetectDevice() = 0;

 protected:
    // instance variables
    bool m_enabled;
    const string m_path;
    ola::io::ConnectedDescriptor *m_socket;
    ola::io::SelectServer *m_ss;
    int byteCounter;
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDWIDGET_H_
