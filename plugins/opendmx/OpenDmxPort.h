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
 * OpenDmxPort.h
 * The Open DMX plugin for lla
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef OPENDMXPORT_H
#define OPENDMXPORT_H

#include <string>
#include <lla/DmxBuffer.h>
#include <llad/Port.h>
#include <OpenDmxThread.h>

namespace lla {
namespace plugin {

using std::string;

class OpenDmxPort: public lla::Port {
  public:
    OpenDmxPort(lla::AbstractDevice *parent, unsigned int id,
                const string &path);
    ~OpenDmxPort();

    bool WriteDMX(const DmxBuffer &buffer);
    // reading isn't supported in the driver
    const DmxBuffer &ReadDMX() const { return m_empty_buffer; }
    bool CanRead() const { return false; }
  private:
    OpenDmxThread *m_thread;
    DmxBuffer m_empty_buffer;
};

} //plugins
} //lla

#endif
