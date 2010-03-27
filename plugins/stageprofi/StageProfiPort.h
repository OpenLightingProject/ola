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
 * StageProfiPort.h
 * The StageProfi plugin for ola
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIPORT_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/stageprofi/StageProfiDevice.h"
#include "plugins/stageprofi/StageProfiWidget.h"

namespace ola {
namespace plugin {
namespace stageprofi {

class StageProfiOutputPort: public BasicOutputPort {
  public:
    StageProfiOutputPort(StageProfiDevice *parent,
                         unsigned int id,
                         StageProfiWidget *widget)
        : BasicOutputPort(parent, id),
          m_widget(widget) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);
    string Description() const { return ""; }

  private:
    StageProfiWidget *m_widget;
};
}  // stageprofi
}  // plugin
}  // ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIPORT_H_
