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

using namespace std;


typedef struct {
	FDListener *listener ;
	int	fd;						// file descriptor
} fd_listener_t ;



#define LLAD_PORT 8898				// port to listen on
#define LLAD_ADDR "127.0.0.1"		// address to bind to

class Network {

	public :
		enum Direction{READ, WRITE};
			
		Network() ;
		~Network() ;
		int init() ;
		int run() ;
		int register_fd(int fd, Network::Direction dir, FDListener *listener, FDManager *manager) ;
		int unregister_fd(int fd, Network::Direction dir) ;
		int register_timeout(int seconds, TimeoutListener *listener) ;
		int read(lla_msg *msg) ;
		int send_msg(lla_msg *msg) ;

	private :
		Network(const Network&);
		Network operator=(const Network&);

		/*
		 * This represents a listener
		 */
		class Listener {
			public:
				Listener(int fd, FDListener *listener, FDManager *manager ) : m_listener(listener) , m_manager(manager), m_fd(fd) {} ;
				FDListener *m_listener ;
				FDManager *m_manager ;
				int	m_fd;
		};

		/*
		 * Represents a timeout we need to check
		 */
		class Timeout {
			public:
				Timeout(int seconds, TimeoutListener *listener) : m_sec(seconds), m_listener(listener) {
					timerclear(&m_tv) ;
				} ;

				/*
				 * Check if this timeout has expired and invoke the action if
				 * is has
				 */
				void check_expiry(struct timeval *now) {
					
					if( timercmp(now, &m_tv, >= ) ) {
						m_listener->timeout_action() ;
						m_tv.tv_sec = now->tv_sec + m_sec ;
						m_tv.tv_usec = now->tv_usec;
					}
				}
				struct timeval m_tv;

			private:
				int m_sec;
				TimeoutListener *m_listener ;


		};
		int m_sd ;
		int fetch_msg_from_client(lla_msg *msg) ;
		void check_timeouts(struct timeval *now) ;
		int get_remaining(struct timeval *now, struct timeval *tv) ;

		vector<Listener*> m_rhandlers_vect ;
		vector<Listener*> m_whandlers_vect ;
		vector<Timeout*>  m_timeouts_vect ;
};

#endif

