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
 * SpiDmxPort.h
 * The DMX through a SPI plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 * Copyright (C) 2014 Richard Ash
 */

#ifndef PLUGINS_SPIDMX_SPIDMXPORT_H_
#define PLUGINS_SPIDMX_SPIDMXPORT_H_

#include <string>

#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "olad/Preferences.h"
#include "plugins/spidmx/SpiDmxDevice.h"
#include "plugins/spidmx/SpiDmxWidget.h"
#include "plugins/spidmx/SpiDmxThread.h"

namespace ola {
namespace plugin {
namespace spidmx {

class SpiDmxInputPort : public ola::BasicInputPort {
 public:
  SpiDmxInputPort(SpiDmxDevice *parent,
                  unsigned int id,
                  PluginAdaptor *plugin_adaptor,
                  SpiDmxWidget *widget,
                  SpiDmxThread *thread)
      : BasicInputPort(parent, id, plugin_adaptor, false),
        m_widget(widget),
        m_thread(thread) {
  }
  ~SpiDmxInputPort() {}

  const DmxBuffer &ReadDMX() const {
    return m_thread->GetDmxInBuffer();
  }

  bool PreSetUniverse(Universe *old_universe, Universe *new_universe) {
    if (!old_universe && new_universe) {
      return m_thread->SetReceiveCallback(NewCallback(
        static_cast<BasicInputPort*>(this),
        &BasicInputPort::DmxChanged));
    }
    if (old_universe && !new_universe) {
      return m_thread->SetReceiveCallback(NULL);
    }
    return true;
  }

  std::string Description() const {
    return m_widget->Description();
  }

 private:
  SpiDmxWidget *m_widget;
  SpiDmxThread *m_thread;

  DISALLOW_COPY_AND_ASSIGN(SpiDmxInputPort);
};

}  // namespace spidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_SPIDMX_SPIDMXPORT_H_
