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
 * network.h
 * The provides operations on a lla_device.
 * Copyright (C) 2005  Simon Newton
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <sys/time.h>

#include <lla/messages.h>
#include <llad/fdlistener.h>
#include <llad/timeoutlistener.h>
#include <llad/fdmanager.h>
#include <vector>
#include <queue>

using namespace std;

/*
typedef struct {
  FDListener *listener;
  int  fd;            // file descriptor
} fd_listener_t;
*/


#define LLAD_PORT 8898        // port to listen on
#define LLAD_ADDR "127.0.0.1"    // address to bind to

class Network {

  public :
    enum Direction{READ, WRITE};

    Network();
    ~Network();
    int init();
    int run();
    int register_fd(int fd, Network::Direction dir, FDListener *listener, FDManager *manager);
    int unregister_fd(int fd, Network::Direction dir);
    int register_timeout(int ms, TimeoutListener *listener, bool recur = true,
                         bool free = false);
    int read(lla_msg *msg);
    int send_msg(lla_msg *msg);

  private :
    Network(const Network&);
    Network operator=(const Network&);

    /*
     * This represents a listener
     */
    class Listener {
      public:
        Listener(int fd, FDListener *listener, FDManager *manager ) : m_listener(listener) , m_manager(manager), m_fd(fd) {};
        FDListener *m_listener;
        FDManager *m_manager;
        int  m_fd;
    };

    typedef struct {
      struct timeval next;
      int interval; // non 0 if this event is recurring
      bool free; // true if we delete the listener once we're done
      TimeoutListener *listener;
    } event_t;

    struct ltevent {
      bool operator()(const event_t &e1, const event_t &e2) const {
        return timercmp(&e1.next, &e2.next, >);
      }
    };

    int m_sd;
    int fetch_msg_from_client(lla_msg *msg);
    struct timeval check_timeouts();

    vector<Listener*> m_rhandlers_vect;
    vector<Listener*> m_whandlers_vect;

    typedef priority_queue<event_t, vector<event_t>, ltevent> event_q_t;
    event_q_t m_event_cbs;
};

#endif

