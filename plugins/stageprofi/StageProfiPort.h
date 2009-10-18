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

#ifndef STAGEPROFIPORT_H
#define STAGEPROFIPORT_H

#include <ola/DmxBuffer.h>
#include <olad/Port.h>
#include "StageProfiDevice.h"

namespace ola {
namespace plugin {

class StageProfiPort: public Port<StageProfiDevice> {
  public:
    StageProfiPort(StageProfiDevice *parent, unsigned int id):
      Port<StageProfiDevice>(parent, id) {};

    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const { return m_empty_buffer; }

  private:
    DmxBuffer m_empty_buffer;
};

} // plugin
} // ola
#endif
