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
 * KaratePort.h
 * The karate plugin for ola
 * Copyright (C) 2005 Simon Newton
 */

#ifndef PLUGINS_KARATE_KARATEPORT_H_
#define PLUGINS_KARATE_KARATEPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/karate/KarateDevice.h"
#include "plugins/karate/KarateThread.h"

namespace ola {
namespace plugin {
namespace karate {

class KarateOutputPort: public BasicOutputPort {
 public:
  KarateOutputPort(KarateDevice *parent,
                   unsigned int id,
                   const std::string &path)
      : BasicOutputPort(parent, id),
        m_thread(path),
        m_path(path) {
    m_thread.Start();
  }

  ~KarateOutputPort() {
    m_thread.Stop();
  }

  std::string Description() const { return "KarateLight at " + m_path; }

  bool WriteDMX(const ola::DmxBuffer &buffer, OLA_UNUSED uint8_t priority) {
    return m_thread.WriteDmx(buffer);
  }

 private:
  KarateThread m_thread;
  std::string m_path;
};
}  // namespace karate
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_KARATE_KARATEPORT_H_
