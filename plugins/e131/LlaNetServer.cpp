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
 * LlaNetServer.cpp
 * Override the ACN NetServer class to integrate with LLA
 * Copyright (C) 2007 Simon Newton
 */

#include <llad/pluginadaptor.h>
#include <llad/timeoutlistener.h>
#include <llad/fdlistener.h>

#include "LlaNetServer.h"

/*
 * glue the NetServers function callbacks with the PluginAdaptor's object 
 * callbacks
 */
class NetServerFDListener : public FDListener {
  public:
    NetServerFDListener(NetServer::callback_fn fn, void *data) :
      m_fn(fn), m_data(data) {}

    int fd_action() {
      if (m_fn)
        m_fn(m_data);
      return 0;
    }
  private:
    NetServer::callback_fn m_fn;
    void *m_data;
};

class NetServerTimeoutListener : public TimeoutListener {
  public:
    NetServerTimeoutListener(NetServer::callback_fn fn, void *data) :
      m_fn(fn), m_data(data) {}

    int timeout_action() {
      if (m_fn)
        m_fn(m_data);
      return 0;
    }
  private:
    NetServer::callback_fn m_fn;
    void *m_data;
};


/*
 * remove all the callbacks
 */
LlaNetServer::~LlaNetServer() {
  map<int, NetServerFDListener*>::iterator iter;

  for(iter = m_lmap.begin(); iter != m_lmap.end(); ++iter)
    delete iter->second;

  m_lmap.clear();
}


/*
 * add a fd callback
 */
int LlaNetServer::add_fd(int fd, callback_fn fn, void *data) {
  NetServerFDListener *l = new NetServerFDListener(fn, data);
  if (m_lmap.find(fd) != m_lmap.end())
    delete m_lmap[fd];

  m_lmap[fd] = l;
  return m_pa->register_fd(fd, PluginAdaptor::READ, l, NULL);
}


/*
 * remove a fd callback
 */
int LlaNetServer::remove_fd(int fd) {

  if (m_lmap.find(fd) != m_lmap.end())
    delete m_lmap[fd];
  m_lmap.erase(fd);

  m_pa->unregister_fd(fd, PluginAdaptor::READ);
  return 0;
}


/*
 * register an event
 */
int LlaNetServer::register_event(int ms, callback_fn fn, void *data) {
  NetServerTimeoutListener *l = new NetServerTimeoutListener(fn, data);

  return m_pa->register_timeout(ms, l);
}


int LlaNetServer::loop_callback(callback_fn fn, void *data) {
  fn = NULL;
  data = NULL;

  return 0;
}
