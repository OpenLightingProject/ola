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
 * The StageProfi LAN Widget.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include <llad/logger.h>
#include "StageProfiWidgetLan.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

static const unsigned int STAGEPROFI_PORT = 10001;

/*
 * Connect to the widget
 */
int StageProfiWidgetLan::connect(const string &path) {
  struct sockaddr_in servaddr;

  m_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (! m_fd) {
    Logger::instance()->log(Logger::CRIT, "StageProfiWidget: could not create socket %s", strerror(errno));
    return -1;
  }

  memset(&servaddr, 0x00, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(STAGEPROFI_PORT);
  inet_aton(path.c_str(), &servaddr.sin_addr);

  if (::connect(m_fd, (struct sockaddr *) &servaddr, sizeof(servaddr))) {
    Logger::instance()->log(Logger::CRIT, "StageProfiWidget: could not connect %s", strerror(errno));
    return -1;
  }
  return 0;
}
