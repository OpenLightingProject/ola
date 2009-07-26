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
 * StageProfiWidgetUsb.h
 * Interface for the StageProfi USB device
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef STAGEPROFIWIDGETUSB_H
#define STAGEPROFIWIDGETUSB_H

#include "StageProfiWidget.h"

namespace ola {
namespace plugin {

class StageProfiWidgetUsb: public StageProfiWidget {
  public:
    StageProfiWidgetUsb(): StageProfiWidget() {}
    ~StageProfiWidgetUsb() {}

    bool Connect(const std::string &ip);
};

} // plugin
} // ola
#endif
