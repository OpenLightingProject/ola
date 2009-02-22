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
 *
 * shownetdevice.h
 * Interface for the shownet device
 * Copyright (C) 2005  Simon Newton
 */

#ifndef SHOWNETDEVICE_H
#define SHOWNETDEVICE_H

#include <llad/Device.h>
#include <lla/select_server/FDListener.h>

#include <shownet/shownet.h>

#include "common.h"

class ShowNetDevice : public Device, public Listener {

  public:
    ShowNetDevice(Plugin *owner, const string &name, class Preferences *prefs);
    ~ShowNetDevice();

    bool Start();
    bool Stop();
    shownet_node get_node() const;
    int get_sd() const;
    int action();
    int SaveConfig() const;
    int configure(void *req, int len);

  private:
    class Preferences *m_preferences;
    shownet_node m_node;
    bool m_enabled;
};

#endif
