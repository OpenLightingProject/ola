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
 * stageprofiwidget.cpp
 * StageProfi Widget
 * Copyright (C) 2006-2007 Simon Newton
 *
 * The StageProfi Usb Widget.
 */

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <llad/logger.h>
#include "StageProfiWidgetUsb.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

/*
 * Connect to the widget
 */
int StageProfiWidgetUsb::connect(const string &path) {
  struct termios newtio;
  m_fd = open(path.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (m_fd == -1) {
    return 1;
  }

  bzero(&newtio, sizeof(newtio)); // clear struct for new port settings
  tcgetattr(m_fd, &newtio);
  cfsetospeed(&newtio, B38400);
  tcsetattr(m_fd,TCSANOW,&newtio);

  return 0;
}

