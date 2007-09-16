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
 * StageOrofiDeviceLan.h
 * Interface for the StageProfi LAN device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef STAGEPROFIWIDGETLAN_H
#define STAGEPROFIWIDGETLAN_H

#include "StageProfiWidget.h"

using namespace std;

#include <string>
#include <stdint.h>

class StageProfiWidgetLan : public StageProfiWidget {

  public:
    StageProfiWidgetLan() : StageProfiWidget() {}
    ~StageProfiWidgetLan() {}

    int connect(const string &ip);

};

#endif
