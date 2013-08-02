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
 * MilInstWidget.cpp
 * This is the base widget class
 * Copyright (C) 2013 Peter Newman
 */

#include <string>

#include "ola/Logging.h"
#include "plugins/milinst/MilInstWidget.h"

namespace ola {
namespace plugin {
namespace milinst {

/*
 * New widget
 */
MilInstWidget::~MilInstWidget() {
  if (m_socket) {
    m_socket->Close();
    delete m_socket;
  }
}

/*
 * Connect to the widget
 */
int MilInstWidget::ConnectToWidget(const std::string &path, speed_t speed) {
  struct termios newtio;

  if (path.empty()) {
    OLA_DEBUG << "No path configured for device, please set one in "
        "ola-milinst.conf";
    return -1;
  }

  int fd = open(path.data(), O_RDWR | O_NONBLOCK | O_NOCTTY);

  if (fd == -1)
    return -1;

  memset(&newtio, 0, sizeof(newtio));  // Clear struct for new port settings
  tcgetattr(fd, &newtio);
  newtio.c_cflag |= (CLOCAL | CREAD);  // Enable read
  newtio.c_cflag |= CS8;  // 8n1
  newtio.c_cflag &= ~CRTSCTS;  // No flow control
  cfsetispeed(&newtio, speed);
  cfsetospeed(&newtio, speed);
  tcsetattr(fd, TCSANOW, &newtio);

  return fd;
}


/*
 * Disconnect from the widget
 */
int MilInstWidget::Disconnect() {
  m_socket->Close();
  return 0;
}
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
