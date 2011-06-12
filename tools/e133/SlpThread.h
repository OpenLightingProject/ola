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

using std::string;

typedef std::vector<string> url_vector;
typedef ola::BaseCallback1<void, bool> slp_registration_callback;
typedef ola::BaseCallback2<void, bool, url_vector*> slp_discovery_callback;

// The interface for the SLPAction classes
class BaseSlpAction {
  public:
    virtual ~BaseSlpAction() {}

    virtual void Perform(SLPHandle handle) = 0;
    virtual void RequestComplete() = 0;

    static const char SERVICE_NAME[];
};


// A SLPAction, templatized by callback type
template <typename callback_type>
class SlpAction: public BaseSlpAction {
  public:
    SlpAction(callback_type *callback)
        : m_ok(false),
          m_callback(callback) {}
    virtual ~SlpAction() {}

    virtual void Perform(SLPHandle handle) = 0;
    virtual void RequestComplete() = 0;


  protected:
    bool m_ok;
    callback_type *m_callback;
};


// The SLP Discovery Action
class SlpDiscoveryAction: public SlpAction<slp_discovery_callback> {
  public:
    SlpDiscoveryAction(slp_discovery_callback *callback,
                       url_vector *urls)
        : SlpAction<slp_discovery_callback>(callback),
          m_urls(urls) {
    }

    void Perform(SLPHandle handle);
    void RequestComplete() {
      m_callback->Run(m_ok, m_urls);
    }

  private:
    url_vector *m_urls;
};


// SLP Register Action
class SlpRegistrationAction: public SlpAction<slp_registration_callback> {
  public:
    SlpRegistrationAction(slp_registration_callback *callback,
                          const string &url,
                          unsigned short lifetime)
        : SlpAction<slp_registration_callback>(callback),
          m_url(url),
          m_lifetime(lifetime) {
    }

    void Perform(SLPHandle handle);
    void RequestComplete() {
      m_callback->Run(m_ok);
    }

  private:
    string m_url;
    unsigned short m_lifetime;
};


// SLP DeRegister Action
class SlpDeRegistrationAction: public SlpAction<slp_registration_callback> {
  public:
    SlpDeRegistrationAction(slp_registration_callback *callback,
                            const string &url)
        : SlpAction<slp_registration_callback>(callback),
          m_url(url) {
    }

    void Perform(SLPHandle handle);
    void RequestComplete() {
      m_callback->Run(m_ok);
    }

  private:
    string m_url;
};


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

    // enqueue discovery request
    void Discover(slp_discovery_callback *callback,
                  url_vector *urls);

    // queue a registration request
    void Register(slp_registration_callback *callback,
                  const string &url,
                  unsigned short lifetime = SLP_LIFETIME_MAXIMUM);

    // queue a de-register request
    void DeRegister(slp_registration_callback *callback,
                    const string &url);

    void *Run();

  private:
    ola::network::SelectServer *m_main_ss;
    ola::network::LoopbackSocket m_outgoing_socket;
    std::queue<BaseSlpAction*> m_incoming_queue;
    std::queue<BaseSlpAction*> m_outgoing_queue;
    pthread_cond_t m_incomming_condition;
    pthread_mutex_t m_incomming_mutex, m_outgoing_mutex;
    bool m_init_ok, m_terminate;
    SLPHandle m_slp_handle;

    void RequestComplete();
    void WakeUpSocket(ola::network::LoopbackSocket *socket);
    void EmptySocket(ola::network::LoopbackSocket *socket);
};

#endif  // TOOLS_E133_SLPTHREAD_H_
