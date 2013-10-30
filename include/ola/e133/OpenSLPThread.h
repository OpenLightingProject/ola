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

#ifndef INCLUDE_OLA_E133_OPENSLPTHREAD_H_
#define INCLUDE_OLA_E133_OPENSLPTHREAD_H_

#include <slp.h>
#include <ola/base/Macro.h>
#include <ola/e133/SLPThread.h>
#include <ola/network/Socket.h>
#include <ola/thread/ExecutorInterface.h>
#include <ola/thread/Thread.h>

#include <memory>
#include <string>

namespace ola {
namespace e133 {

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
                            uint16_t lifetime);
    void DeRegisterSLPService(RegistrationCallback *callback,
                              const string& url);
    void SLPServerInfo(ServerInfoCallback *callback);

  private:
    bool m_init_ok;
    SLPHandle m_slp_handle;

    DISALLOW_COPY_AND_ASSIGN(OpenSLPThread);
};
}  // namespace e133
}  // namespace ola
#endif  // INCLUDE_OLA_E133_OPENSLPTHREAD_H_
