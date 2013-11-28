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
 * RenardPort.h
 * The Renard plugin for ola
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDPORT_H_
#define PLUGINS_RENARD_RENARDPORT_H_

#include <string>

#include "plugins/renard/RenardDevice.h"
#include "plugins/renard/RenardWidget.h"

namespace ola {
namespace plugin {
namespace renard {

class RenardOutputPort: public BasicOutputPort {
  public:
    RenardOutputPort(RenardDevice *parent,
                     unsigned int id,
                     RenardWidget *widget)
        : BasicOutputPort(parent, id),
          m_widget(widget) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return m_widget->GetPath(); }

  private:
    RenardWidget *m_widget;
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDPORT_H_
