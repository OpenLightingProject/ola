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
 * MilInstWidget.h
 * Interface for the milinst widget
 * Copyright (C) 2013 Peter Newman
 */

#ifndef PLUGINS_MILINST_MILINSTWIDGET_H_
#define PLUGINS_MILINST_MILINSTWIDGET_H_

#include <fcntl.h>
#include <termios.h>
#include <string>

#include "ola/io/SelectServer.h"
#include "ola/DmxBuffer.h"

namespace ola {
namespace plugin {
namespace milinst {

using ola::io::ConnectedDescriptor;
using ola::io::SelectServer;

class MilInstWidget {
  public:
    static int ConnectToWidget(const std::string &path, speed_t speed = B9600);

    explicit MilInstWidget(const std::string &path):
      m_enabled(false),
      m_path(path),
      m_socket(NULL),
      m_ss(NULL) {}
    virtual ~MilInstWidget();

    // these methods are for communicating with the device
    virtual bool Connect() = 0;
    int Disconnect();
    ConnectedDescriptor *GetSocket() { return m_socket; }
    string GetPath() { return m_path; }
    virtual bool SendDmx(const DmxBuffer &buffer) const = 0;
    virtual bool DetectDevice() = 0;

  protected:
    virtual int SetChannel(unsigned int chan, uint8_t val) const = 0;

    // instance variables
    bool m_enabled;
    const string m_path;
    ConnectedDescriptor *m_socket;
    SelectServer *m_ss;
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTWIDGET_H_
