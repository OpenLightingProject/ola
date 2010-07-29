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
 * usbpro-common.cpp
 * Copyright (C) 2010 Simon Newton
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <ola/Logging.h>

#include <string>

#include "tools/usbpro/usbpro-common.h"

using std::string;

/*
 * Open the widget device
 */
int ConnectToWidget(const string &path) {
  struct termios newtio;
  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1) {
    OLA_WARN << "Failed to open " << path << " " << strerror(errno);
    return -1;
  }

  bzero(&newtio, sizeof(newtio));  // clear struct for new port settings
  cfsetispeed(&newtio, B115200);
  cfsetospeed(&newtio, B115200);
  tcsetattr(fd, TCSANOW, &newtio);
  return fd;
}
