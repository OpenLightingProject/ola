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
 * MilInstWidget1553.h
 * Interface for the MilInst 1-553 device
 * Copyright (C) 2013 Peter Newman
 */

#ifndef PLUGINS_MILINST_MILINSTWIDGET1553_H_
#define PLUGINS_MILINST_MILINSTWIDGET1553_H_

#include <string>

#include "plugins/milinst/MilInstWidget.h"

namespace ola {
namespace plugin {
namespace milinst {

class MilInstWidget1553: public MilInstWidget {
 public:
  explicit MilInstWidget1553(const std::string &path, Preferences *preferences);
  ~MilInstWidget1553() {}

  bool Connect();
  bool DetectDevice();
  bool SendDmx(const DmxBuffer &buffer);
  std::string Type() { return "Milford Instruments 1-553 Widget"; }

  void SocketReady();

  std::string Description() {
    std::ostringstream str;
    str << GetPath() << ", " << Type();
    if (m_channels != CHANNELS_512) {
      str << ", " << m_channels << " channels";
    }
    return str.str();
  }

 protected:
  int SetChannel(unsigned int chan, uint8_t val);
  int Send(const DmxBuffer &buffer);

  static const uint8_t MILINST_1553_LOAD_COMMAND = 0x01;
  static const uint8_t MILINST_1553_SET_CHANNEL_COUNT_COMMAND = 0x02;
  static const uint8_t MILINST_1553_GET_CHANNEL_COUNT_COMMAND = 0x06;

  static const uint8_t MILINST_1553_END_BYTE = 0x55;

  typedef enum {
    LOAD,
    GET_CHANNEL_COUNT,
    SET_CHANNEL_COUNT,
    _LAST_STATE
  } receive_states;

  static const speed_t DEFAULT_BAUDRATE;

  static const uint16_t CHANNELS_128;
  static const uint16_t CHANNELS_256;
  static const uint16_t CHANNELS_512;
  static const uint16_t DEFAULT_CHANNELS;

 private:
  class Preferences *m_preferences;
  uint16_t m_channels;

  receive_states m_current_receive_state;

  // Per widget options
  std::string BaudRateKey() const;
  std::string ChannelsKey() const;

  void SetWidgetDefaults();
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTWIDGET1553_H_
