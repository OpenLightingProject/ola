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
 * A thread to encapsulate all E1.33 SLP operations.
 *
 * Brief overview:
 *   Like the name implies, the SLPThread starts up a new thread to handle SLP
 *   operations (openslp docs indicate asynchronous operations aren't
 *   supported and even if they were, you can't have more than one operation
 *   pending at once so the need for serialization still exists).
 *
 *   Each call to Discover(), Register() & DeRegister() executes the callback
 *   in the SLP thread.
 *
 *   Summary: The callbacks passed to the SLP methods are run in the
 *   thread that contains the SelectServer passed to the SlpThread constructor.
 */

#include <slp.h>
#include <ola/Callback.h>
#include <ola/thread/Thread.h>
#include <ola/network/SelectServer.h>
#include <ola/network/Socket.h>

#include <map>
#include <queue>
#include <string>
#include <vector>


#ifndef TOOLS_E133_SLPTHREAD_H_
#define TOOLS_E133_SLPTHREAD_H_

using std::string;

typedef std::vector<string> url_vector;
typedef ola::BaseCallback1<void, bool> slp_registration_callback;
typedef ola::Callback2<void, bool, const url_vector&> slp_discovery_callback;


/**
 * A thread which handles SLP events.
 */
class SlpThread: public ola::thread::Thread {
  public:
    explicit SlpThread(ola::network::SelectServer *ss,
                       slp_discovery_callback *discovery_callback = NULL,
                       unsigned int refresh_time = DISCOVERY_INTERVAL_S);
    ~SlpThread();

    bool Init();
    bool Start();
    bool Join(void *ptr = NULL);
    void Cleanup();

    // enqueue discovery request
    bool Discover();

    // queue a registration request
    void Register(slp_registration_callback *callback,
                  const string &url,
                  unsigned short lifetime = SLP_LIFETIME_MAXIMUM);

    // queue a de-register request
    void DeRegister(slp_registration_callback *callback,
                    const string &url);


    void *Run();

  private:
    typedef struct {
      unsigned short lifetime;
      ola::network::timeout_id timeout;
    } url_registration_state;
    typedef std::map<string, url_registration_state>  url_state_map;

    ola::network::SelectServer m_ss;
    ola::network::SelectServer *m_main_ss;
    bool m_init_ok;
    unsigned int m_refresh_time;
    SLPHandle m_slp_handle;
    slp_discovery_callback *m_discovery_callback;
    ola::network::timeout_id m_discovery_timeout;
    url_state_map m_url_map;

    void RequestComplete();
    void AddToOutgoingQueue(ola::BaseCallback0<void> *callback);

    void DiscoveryRequest();
    void RegisterRequest(slp_registration_callback *callback,
                         const string url,
                         unsigned short lifetime);
    bool PerformRegistration(const string &url,
                             unsigned short lifetime,
                             ola::network::timeout_id *timeout);
    void DeregisterRequest(slp_registration_callback *callback,
                           const string url);
    void DiscoveryActionComplete(bool ok, url_vector *urls);
    void SimpleActionComplete(slp_registration_callback *callback,
                              bool ok);

    void DiscoveryTriggered();
    void RegistrationTriggered(string url);

    // How often to repeat discovery
    static const unsigned short DISCOVERY_INTERVAL_S = 60;
    // the minimum lifetime we'll ever allow, may be more due to the
    // min-refresh-interval attribute sent by DAs
    static const unsigned short MIN_LIFETIME;
    // This is the cycle period of SLP aging. Registrations must be renewed
    // this many seconds before the registration is set to expire.
    // See http://opendmx.net/index.php/Open_SLP_Notes
    static const uint16_t SLPD_AGING_TIME_S = 15;
};

#endif  // TOOLS_E133_SLPTHREAD_H_
