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

#ifndef STAGEPROFIWIDGET_H
#define STAGEPROFIWIDGET_H

using namespace std;

#include <string>
#include <stdint.h>

class StageProfiWidget {

  public:
    StageProfiWidget() {};
    virtual ~StageProfiWidget() {};

    // these methods are for communicating with the device
    virtual int connect(const string &path) = 0;
    int disconnect();
    int fd() {return m_fd;}
    int send_dmx(uint8_t *buf, unsigned int len) const;
    int recv();

  protected:
    int send_255(unsigned int start, uint8_t *buf, unsigned int len) const;

    // instance variables
    int m_fd;            // file descriptor
    bool m_enabled;          // are we enabled

  private:
    int do_recv();
};

#endif
