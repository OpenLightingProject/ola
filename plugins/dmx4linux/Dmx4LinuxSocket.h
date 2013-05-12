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
 * Dmx4LinuxSocket.h
 * Interface for the dmx4linux socket class
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef PLUGINS_DMX4LINUX_DMX4LINUXSOCKET_H_
#define PLUGINS_DMX4LINUX_DMX4LINUXSOCKET_H_

#include "ola/network/Socket.h"

namespace ola {
namespace plugin {
namespace dmx4linux {

class Dmx4LinuxSocket: public ola::network::DeviceDescriptor {
  public:
    explicit Dmx4LinuxSocket(int fd): ola::network::DeviceDescriptor(fd) {}
  protected:
    virtual bool IsClosed() const {return false;}
};
}  // namespace dmx4linux
}  // namespace plugin
}  // namespace ola

#endif  // PLUGINS_DMX4LINUX_DMX4LINUXSOCKET_H_
