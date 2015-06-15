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

class MilInstWidget {
 public:
  static int ConnectToWidget(const std::string &path, speed_t speed = B9600);

  explicit MilInstWidget(const std::string &path)
      : m_enabled(false),
        m_path(path),
        m_socket(NULL) {}

  virtual ~MilInstWidget();

  // these methods are for communicating with the device
  virtual bool Connect() = 0;
  int Disconnect();
  ola::io::ConnectedDescriptor *GetSocket() { return m_socket; }
  std::string GetPath() { return m_path; }
  virtual std::string Type() = 0;

  // Virtual so we can override it locally if desired
  virtual std::string Description() {
    std::ostringstream str;
    str << GetPath() << ", " << Type();
    return str.str();
  }

  virtual bool SendDmx(const DmxBuffer &buffer) = 0;
  virtual bool DetectDevice() = 0;

 protected:
  virtual int SetChannel(unsigned int chan, uint8_t val) = 0;

  // instance variables
  bool m_enabled;
  const std::string m_path;
  ola::io::ConnectedDescriptor *m_socket;
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTWIDGET_H_
