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
 * usbproport.h
 * The UsbPro plugin for lla
 * Copyright (C) 2006  Simon Newton
 */

#ifndef USBPROPORT_H
#define USBPROPORT_H

#include <llad/port.h>

class UsbProPort : public Port {

  public:
    UsbProPort(Device *parent, int id) : Port(parent, id) {};

    int write(uint8_t *data, int length);
    int read(uint8_t *data, int length);
    int set_universe(Universe *uni);
    int can_read() const;
    int can_write() const;

};

#endif
