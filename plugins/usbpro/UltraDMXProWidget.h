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
 * UltraDMXProWidget.h
 * The DMXKing Ultra DMX Pro Widget.
 * This is similar to the Enttec Usb Pro, but it has two output ports.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ULTRADMXPROWIDGET_H_
#define PLUGINS_USBPRO_ULTRADMXPROWIDGET_H_

#include <deque>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/thread/SchedulerInterface.h"
#include "plugins/usbpro/GenericUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * An Ultra DMX Pro Widget
 */
class UltraDMXProWidget: public GenericUsbProWidget {
  public:
    UltraDMXProWidget(ola::thread::SchedulerInterface *scheduler,
                       ola::network::ConnectedDescriptor *descriptor);
    ~UltraDMXProWidget() {}
    void Stop() { GenericStop(); }

    bool SendDMX(const DmxBuffer &buffer);
    bool SendSecondaryDMX(const DmxBuffer &buffer);

  private:
    bool SendDMXWithLabel(uint8_t label, const DmxBuffer &data);

    static const uint8_t DMX_PRIMARY_PORT = 100;
    static const uint8_t DMX_SECONDARY_PORT = 101;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_ULTRADMXPROWIDGET_H_
