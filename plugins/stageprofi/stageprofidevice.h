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
 * stageprofidevice.h
 * Interface for the stageprofi device
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef STAGEPROFIDEVICE_H
#define STAGEPROFIDEVICE_H

#include <string>
#include <stdint.h>
#include <llad/device.h>
#include <llad/fdlistener.h>

class StageProfiDevice : public Device, public FDListener {

  public:

    StageProfiDevice(Plugin *owner, const string &name, const string &dev_path);
    ~StageProfiDevice();

    int start();
    int stop();
    int get_sd() const;
    int fd_action();
    int save_config() const;
    class LlaDevConfMsg *configure(const uint8_t *request, int reql);
    int send_dmx(uint8_t *data, int len);

  private:

    // instance variables
    string m_path;
    bool m_enabled;        // are we enabled
    class StageProfiWidget *m_widget;
};

#endif
