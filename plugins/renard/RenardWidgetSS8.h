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
 * RenardWidgetSS8.h
 * Interface for the Renard devices, using multiples of 8 channels
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDWIDGETSS8_H_
#define PLUGINS_RENARD_RENARDWIDGETSS8_H_

#include <string>

#include "plugins/renard/RenardWidget.h"

namespace ola {
namespace plugin {
namespace renard {

class RenardWidgetSS8: public RenardWidget {
  public:
    explicit RenardWidgetSS8(const std::string &path,
                             int dmxOffset,
                             int channels,
                             unsigned int baudrate): RenardWidget(path),
        m_dmxOffset(dmxOffset),
        m_channels(channels),
        m_baudrate(baudrate),
        m_startAddress(RENARD_START_ADDRESS) {}
    ~RenardWidgetSS8() {}

    bool Connect();
    bool DetectDevice();
    bool SendDmx(const DmxBuffer &buffer);
  private:
    int m_dmxOffset;
    int m_channels;
    unsigned int m_baudrate;
    uint8_t m_startAddress;
    
    static const uint8_t RENARD_START_ADDRESS;
    static const uint8_t RENARD_COMMAND_PAD;
    static const uint8_t RENARD_COMMAND_PACKET_START;
    static const uint8_t RENARD_COMMAND_ESCAPE;
    static const uint8_t RENARD_CHANNELS_IN_BANK;
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDWIDGETSS8_H_
