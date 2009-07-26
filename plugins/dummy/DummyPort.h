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
 * DummyPort_h
 * The interface to the Dummy port
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef DUMMYPORT_H
#define DUMMYPORT_H

#include <ola/DmxBuffer.h>
#include <olad/Port.h>
#include "DummyDevice.h"

namespace ola {
namespace plugin {

class DummyPort: public Port<DummyDevice> {
  public:
    DummyPort(DummyDevice *parent, unsigned int id):
      Port<DummyDevice>(parent, id) {}

    bool CanRead() const { return false; }
    bool WriteDMX(const DmxBuffer &buffer);
    const DmxBuffer &ReadDMX() const { return m_buffer; }
    string Description() const { return "Dummy Port"; }

  private:
    DmxBuffer m_buffer;
};

} //plugin
} //ola
#endif
