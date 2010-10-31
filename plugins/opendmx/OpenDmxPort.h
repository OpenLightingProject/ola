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
 * The Open DMX plugin for ola
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_OPENDMX_OPENDMXPORT_H_
#define PLUGINS_OPENDMX_OPENDMXPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/opendmx/OpenDmxDevice.h"
#include "plugins/opendmx/OpenDmxThread.h"

namespace ola {
namespace plugin {
namespace opendmx {

using std::string;

class OpenDmxOutputPort: public BasicOutputPort {
  public:
    OpenDmxOutputPort(OpenDmxDevice *parent,
                      unsigned int id,
                      const string &path)
        : BasicOutputPort(parent, id),
          m_thread(path),
          m_path(path) {
      m_thread.Start();
    }

    ~OpenDmxOutputPort() {
      m_thread.Stop();
    }

    string Description() const { return "Open Dmx at " + m_path; }

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_thread.WriteDmx(buffer);
      (void) priority;
    }

  private:
    OpenDmxThread m_thread;
    string m_path;
};
}  // opendmx
}  // plugins
}  // ola
#endif  // PLUGINS_OPENDMX_OPENDMXPORT_H_
