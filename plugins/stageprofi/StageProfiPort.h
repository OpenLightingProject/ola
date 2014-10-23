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
 * StageProfiPort.h
 * The StageProfi plugin for ola
 * Copyright (C) 2006 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIPORT_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIPORT_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "olad/Port.h"
#include "plugins/stageprofi/StageProfiDevice.h"

namespace ola {
namespace plugin {
namespace stageprofi {

class StageProfiWidget;

/**
 * @brief An OutputPort for the StageProfi Device.
 */
class StageProfiOutputPort: public BasicOutputPort {
 public:
  /**
   * @brief Create a new StageProfi Output Port.
   * @param parent the StageProfiDevice this port belongs to
   * @param id the port-id.
   * @param widget the StageProfiWidget to use for this port, ownership is not
   *   transferred.
   */
  StageProfiOutputPort(StageProfiDevice *parent,
                       unsigned int id,
                       StageProfiWidget *widget);

  bool WriteDMX(const DmxBuffer &buffer, uint8_t priority);

  std::string Description() const;

 private:
  StageProfiWidget *m_widget;
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIPORT_H_
