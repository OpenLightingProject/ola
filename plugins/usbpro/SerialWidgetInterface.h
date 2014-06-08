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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * SerialWidgetInterface.h
 * The base class for USB Serial widgets. These are devices that appear as a
 * serial (COM) port, most of them use the FTDI chip.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_SERIALWIDGETINTERFACE_H_
#define PLUGINS_USBPRO_SERIALWIDGETINTERFACE_H_

#include <stdint.h>
#include <ola/Callback.h>
#include <string>
#include "ola/io/Descriptor.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * The SerialWidgetInterface.
 */
class SerialWidgetInterface {
 public:
    SerialWidgetInterface() {}
    virtual ~SerialWidgetInterface() {}

    // The Stop() method should clear out any callbacks
    virtual void Stop() = 0;
    virtual ola::io::ConnectedDescriptor *GetDescriptor() const = 0;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_SERIALWIDGETINTERFACE_H_
