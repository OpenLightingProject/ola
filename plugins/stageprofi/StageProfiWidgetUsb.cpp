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
 * StageProfiWidgetUsb.cpp
 * The StageProfi Usb Widget.
 * Copyright (C) 2006-2009 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <termios.h>


#include <lla/select_server/Socket.h>
#include "StageProfiWidgetUsb.h"

namespace lla {
namespace plugin {

/*
 * Connect to the widget
 */
bool StageProfiWidgetUsb::Connect(const std::string &path) {
  struct termios newtio;

  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1)
    return false;

  bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
  tcgetattr(fd, &newtio);
  cfsetospeed(&newtio, B38400);
  tcsetattr(fd, TCSANOW, &newtio);
  m_socket = new ConnectedSocket(fd, fd);
  return true;
}

} // plugin
} // lla
