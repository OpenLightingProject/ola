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
  bool SendDmx(const DmxBuffer &buffer) const;
  std::string Type() { return "Milford Instruments 1-553 Widget"; }

 protected:
  int SetChannel(unsigned int chan, uint8_t val) const;
  int Send(const DmxBuffer &buffer) const;

  static const uint8_t MILINST_1553_LOAD_COMMAND = 0x01;

  static const char BAUDRATE_9600[];
  static const char BAUDRATE_19200[];
  static const speed_t DEFAULT_BAUDRATE;

  static const char CHANNELS_128[];
  static const char CHANNELS_256[];
  static const char CHANNELS_512[];
  static const uint16_t DEFAULT_CHANNELS;

 private:
  class Preferences *m_preferences;
  uint16_t m_channels;

  // Per widget options
  string BaudRateKey() const;
  string ChannelsKey() const;

  void SetWidgetDefaults();
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTWIDGET1553_H_
