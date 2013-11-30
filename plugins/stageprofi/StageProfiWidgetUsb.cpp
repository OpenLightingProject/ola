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
 * StageProfiWidgetUsb.cpp
 * The StageProfi Usb Widget.
 * Copyright (C) 2006-2009 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <string>

#include "ola/Callback.h"
#include "ola/io/Descriptor.h"
#include "ola/io/IOUtils.h"
#include "plugins/stageprofi/StageProfiWidgetUsb.h"

namespace ola {
namespace plugin {
namespace stageprofi {

/*
 * Connect to the widget
 */
bool StageProfiWidgetUsb::Connect(const std::string &path) {
  struct termios newtio;

  int fd;
  if (!ola::io::Open(path, O_RDWR | O_NONBLOCK | O_NOCTTY, &fd)) {
    return false;
  }

  memset(&newtio, 0, sizeof(newtio));  // clear struct for new port settings
  tcgetattr(fd, &newtio);
  cfsetospeed(&newtio, B38400);
  tcsetattr(fd, TCSANOW, &newtio);
  m_socket = new ola::io::DeviceDescriptor(fd);
  m_socket->SetOnData(
      NewCallback<StageProfiWidget>(this, &StageProfiWidget::SocketReady));
  m_device_path = path;
  return true;
}
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
