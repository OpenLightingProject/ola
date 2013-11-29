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
 * StageProfiWidgetLan.h
 * Interface for the StageProfi LAN device
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIWIDGETLAN_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIWIDGETLAN_H_

#include <string>
#include "plugins/stageprofi/StageProfiWidget.h"

namespace ola {
namespace plugin {
namespace stageprofi {

class StageProfiWidgetLan: public StageProfiWidget {
 public:
    StageProfiWidgetLan(): StageProfiWidget() {}
    ~StageProfiWidgetLan() {}

    bool Connect(const std::string &ip);
 private:
    static const uint16_t STAGEPROFI_PORT = 10001;
};
}  // namespace stageprofi
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIWIDGETLAN_H_
