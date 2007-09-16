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
 * dmx4linuxdevice.h
 * Interface for the dmx4linux device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef DMX4LINUXDEVICE_H
#define DMX4LINUXDEVICE_H

#include <string>
#include <stdint.h>
#include <llad/device.h>

class Dmx4LinuxDevice : public Device {

  public:

    Dmx4LinuxDevice(class Dmx4LinuxPlugin *owner, const string &name);
    ~Dmx4LinuxDevice();

    int start();
    int stop();
    int save_config() const;
    class LlaDevConfMsg *configure(const uint8_t *request, int reql);
    int send_dmx(int d4l_uni, uint8_t *data, int len);

  private:

    // instance variables
    class Dmx4LinuxPlugin *m_plugin; //
    bool m_enabled;        // are we enabled
};

#endif
