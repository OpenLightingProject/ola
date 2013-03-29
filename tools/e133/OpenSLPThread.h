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
 * OpenSLPThread.h
 * Copyright (C) 2011 Simon Newton
 *
 * An implementation of BaseSLPThread that uses openslp.
 */

#include <slp.h>
#include <ola/thread/Thread.h>
#include <ola/network/Socket.h>
#include <ola/thread/ExecutorInterface.h>

#include <memory>
#include <string>

#include "tools/e133/SLPThread.h"

#ifndef TOOLS_E133_OPENSLPTHREAD_H_
#define TOOLS_E133_OPENSLPTHREAD_H_

using std::string;
using std::auto_ptr;

class OpenSLPThread: public BaseSLPThread {
  public:
    // Ownership of the discovery_callback is transferred.
    OpenSLPThread(
        ola::thread::ExecutorInterface *ss,
        unsigned int discovery_interval = DEFAULT_DISCOVERY_INTERVAL_SECONDS);
    ~OpenSLPThread();

    bool Init();
    void Cleanup();

  protected:
    void RunDiscovery(InternalDiscoveryCallback *callback,
                      const string &service);
    void RegisterSLPService(RegistrationCallback *callback,
                            const string& url,
                            unsigned short lifetime);
    void DeRegisterSLPService(RegistrationCallback *callback,
                              const string& url);

  private:
    bool m_init_ok;
    SLPHandle m_slp_handle;
};
#endif  // TOOLS_E133_OPENSLPTHREAD_H_
