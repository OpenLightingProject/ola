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
 * LlaNetServer.h
 * Override the libacn NetServer to interface with LLA
 * Copyright (C) 2007 Simon Newton
 */

#ifndef LLANETSERVER_H
#define LLANETSERVER_H

#include <string>
#include <map>
#include <vector>
#include <acn/NetServer.h>

using namespace std;

class LlaNetServer : public NetServer {

  public:
    LlaNetServer(const PluginAdaptor *pa):
      NetServer(), m_pa(pa) {}
    ~LlaNetServer();

    int add_fd(int fd, callback_fn fn, void *data);
    int remove_fd(int fd);

    int register_event(int ms, callback_fn fn, void *data);

    int loop_callback(callback_fn fn, void *data);

    int run() { return 0; }

  private:
    const class PluginAdaptor *m_pa;
    map<int, class NetServerListener*> m_lmap;
    vector<class NetServerListener*> m_loop_ls;
};

#endif

