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
 * MilInstWidget1463.h
 * Interface for the MilInst 1-463 device
 * Copyright (C) 2013 Peter Newman
 */

#ifndef PLUGINS_MILINST_MILINSTWIDGET1463_H_
#define PLUGINS_MILINST_MILINSTWIDGET1463_H_

#include <string>

#include "plugins/milinst/MilInstWidget.h"

namespace ola {
namespace plugin {
namespace milinst {

class MilInstWidget1463: public MilInstWidget {
 public:
  explicit MilInstWidget1463(const std::string &path): MilInstWidget(path) {}
  ~MilInstWidget1463() {}

  bool Connect();
  bool DetectDevice();
  bool SendDmx(const DmxBuffer &buffer) const;
  std::string Type() { return "Milford Instruments 1-463 Widget"; }

 protected:
  int SetChannel(unsigned int chan, uint8_t val) const;
  int Send112(const DmxBuffer &buffer) const;

  // This interface can only transmit 112 channels
  enum { DMX_MAX_TRANSMIT_CHANNELS = 112 };
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTWIDGET1463_H_
