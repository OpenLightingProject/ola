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
 * FtdiDmxPort.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#ifndef PLUGINS_FTDIDMX_FTDIDMXPORT_H_
#define PLUGINS_FTDIDMX_FTDIDMXPORT_H_

#include <string>

#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "olad/Preferences.h"
#include "plugins/ftdidmx/FtdiDmxDevice.h"
#include "plugins/ftdidmx/FtdiWidget.h"
#include "plugins/ftdidmx/FtdiDmxThread.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

class FtdiDmxOutputPort : public ola::BasicOutputPort {
 public:
    FtdiDmxOutputPort(FtdiDmxDevice *parent,
                      FtdiInterface *interface,
                      unsigned int id,
                      unsigned int freq)
      : BasicOutputPort(parent, id, true, true),
          m_interface(interface),
          m_thread(interface, freq) {
      m_thread.Start();
    }
    ~FtdiDmxOutputPort() {
      m_thread.Stop();
      delete m_interface;
    }

    bool WriteDMX(const ola::DmxBuffer &buffer, uint8_t) {
      return m_thread.WriteDMX(buffer);
    }

    std::string Description() const { return m_interface->Description(); }

 private:
    FtdiInterface *m_interface;
    FtdiDmxThread m_thread;
};
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIDMXPORT_H_
