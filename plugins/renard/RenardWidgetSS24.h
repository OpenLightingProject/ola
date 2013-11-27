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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RenardWidgetSS24.h
 * Interface for the Renard SS24 device
 * Copyright (C) 2013 Hakan Lindestaf
 */

#ifndef PLUGINS_RENARD_RENARDWIDGETSS24_H_
#define PLUGINS_RENARD_RENARDWIDGETSS24_H_

#include <string>

#include "plugins/renard/RenardWidget.h"

namespace ola {
namespace plugin {
namespace renard {

class RenardWidgetSS24: public RenardWidget {
  public:
    explicit RenardWidgetSS24(const std::string &path): RenardWidget(path) {}
    ~RenardWidgetSS24() {}

    bool Connect();
    bool DetectDevice();
    bool SendDmx(const DmxBuffer &buffer);
  protected:
    int Send24(const DmxBuffer &buffer);

    // This interface can only transmit 24 channels
    enum { DMX_MAX_TRANSMIT_CHANNELS = 24 };
};
}  // namespace renard
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_RENARD_RENARDWIDGETSS24_H_
