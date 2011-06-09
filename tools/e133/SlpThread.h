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
 * SlpThread.h
 * Copyright (C) 2011 Simon Newton
 *
 * Encapsulate all the SLP operations.
 * libslp is blocking, so we run this all from a separate thread.
 */

#include <slp.h>
#include <ola/Callback.h>
#include <ola/OlaThread.h>
#include <ola/network/SelectServer.h>
#include <ola/network/Socket.h>

#include <string>
#include <vector>
#include <queue>


#ifndef TOOLS_E133_SLPTHREAD_H_
#define TOOLS_E133_SLPTHREAD_H_

typedef std::vector<std::string> url_vector;
typedef ola::BaseCallback2<void, bool, url_vector*> slp_discovery_callback;


/**
 * A thread which handles SLP events.
 */
class SlpThread: public ola::OlaThread {
  public:
    explicit SlpThread(ola::network::SelectServer *ss);
    ~SlpThread();

    bool Init();
    bool Start();
    bool Join(void *ptr = NULL);

    // enqueue a request for discovery
    void Discover(slp_discovery_callback *callback,
                  url_vector *urls);

    void *Run();

  private:
    typedef struct {
      slp_discovery_callback *callback;
      url_vector *urls;
    } pending_action;

    typedef struct {
      slp_discovery_callback *callback;
      bool ok;
      url_vector *urls;
    } completed_action;

    ola::network::SelectServer m_ss;
    ola::network::SelectServer *m_main_ss;
    ola::network::LoopbackSocket m_incoming_socket, m_outgoing_socket;
    std::queue<pending_action> m_incoming_queue;
    std::queue<completed_action> m_outgoing_queue;
    pthread_mutex_t m_incomming_mutex;
    pthread_mutex_t m_outgoing_mutex;
    bool m_init_ok;
    SLPHandle m_slp_handle;

    void NewRequest();
    void RequestComplete();
    void WakeUpSocket(ola::network::LoopbackSocket *socket);
    void EmptySocket(ola::network::LoopbackSocket *socket);
};

#endif  // TOOLS_E133_SLPTHREAD_H_
