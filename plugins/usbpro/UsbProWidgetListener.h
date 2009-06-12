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
 * UsbProWidgetListener.h
 * The interface for the UsbPro Widget Listener class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef USBPRODWIDGETLISTENER_H
#define USBPRODWIDGETLISTENER_H

namespace lla {
namespace usbpro {

enum { SERIAL_NUMBER_LENGTH = 4 };

class UsbProWidgetListener {
  public :
    UsbProWidgetListener() {};
    virtual ~UsbProWidgetListener() {}
    virtual void HandleWidgetDmx() {}
    virtual void HandleWidgetParameters(uint8_t firmware,
                                        uint8_t firmware_high,
                                        uint8_t break_time,
                                        uint8_t mab_time,
                                        uint8_t rate) {}
    virtual void HandleWidgetSerial(
        const uint8_t serial[SERIAL_NUMBER_LENGTH]) {}
    virtual void HandleFirmwareReply(bool success) {}

  private:
    UsbProWidgetListener(const UsbProWidgetListener&);
    UsbProWidgetListener& operator=(const UsbProWidgetListener&);
};

} // usbpro
} //lla
#endif
