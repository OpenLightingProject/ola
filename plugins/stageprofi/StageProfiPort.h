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
 * The StageProfi plugin for lla
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef STAGEPROFIPORT_H
#define STAGEPROFIPORT_H

#include <lla/DmxBuffer.h>
#include <llad/Port.h>

namespace lla {

class AbstractDevice;

namespace plugin {

class StageProfiPort: public Port {
  public:
    StageProfiPort(AbstractDevice *parent, unsigned int id):
      Port(parent, id) {};

    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const { return m_empty_buffer; }

    bool CanRead() const { return false; }
  private:
    DmxBuffer m_empty_buffer;
};

} // plugin
} // lla
#endif
