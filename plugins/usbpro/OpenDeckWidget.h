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
 * OpenDeckWidget.h
 * The OpenDeck Widget.
 * This is similar to Ultra DMX Pro, but uses diff mode protocol for sending data.
 * Copyright (C) 2022 Igor Petrovic
 */

#ifndef PLUGINS_USBPRO_OPENDECKWIDGET_H_
#define PLUGINS_USBPRO_OPENDECKWIDGET_H_

#include <deque>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "olad/TokenBucket.h"
#include "plugins/usbpro/GenericUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * The OpenDeck Widget
 */
class OpenDeckWidget: public GenericUsbProWidget {
 public:
    explicit OpenDeckWidget(ola::io::ConnectedDescriptor *descriptor);
    ~OpenDeckWidget() {}
    void Stop() { GenericStop(); }

    bool SendDMX(const DmxBuffer &buffer,
                 const TokenBucket &bucket,
                 const TimeStamp *wake_time);

 private:
    static const size_t MAX_DIFF_CHANNELS = 128;
    static const uint8_t DMX_SLOT_VALUE_DIFF_LABEL = 80;
    DmxBuffer internal_buffer;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_OPENDECKWIDGET_H_
