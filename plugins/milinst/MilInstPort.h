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
 * MilInstPort.h
 * The MilInst plugin for ola
 * Copyright (C) 2013 Peter Newman
 */

#ifndef PLUGINS_MILINST_MILINSTPORT_H_
#define PLUGINS_MILINST_MILINSTPORT_H_

#include <string>

#include "plugins/milinst/MilInstDevice.h"
#include "plugins/milinst/MilInstWidget.h"

namespace ola {
namespace plugin {
namespace milinst {

class MilInstOutputPort: public BasicOutputPort {
 public:
  MilInstOutputPort(MilInstDevice *parent,
                    unsigned int id,
                    MilInstWidget *widget)
      : BasicOutputPort(parent, id),
        m_widget(widget) {}

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
  std::string Description() const { return m_widget->Description(); }

 private:
  MilInstWidget *m_widget;
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTPORT_H_
