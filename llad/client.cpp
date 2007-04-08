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
 * client.cpp
 * Represents a client connection to llad
 * Copyright (C) 2005 Simon Newton
 */

#include <llad/logger.h>
#include "client.h"

map<int ,Client *> Client::cli_map;

/*
 * Create a new client
 *
 * @param port  the port the client is on
 */
Client::Client(int port) {
  m_port = port;
}

/*
 * Destroy this client
 *
 */
Client::~Client() {
  cli_map.erase(m_port);
}

/*
 * Get this port of this client
 *
 * @return the port of the client
 */
int Client::get_port() {
  return m_port;
}


// Class Methods
//-----------------------------------------------------------------------------

/*
 * Lookup a client from the port number
 *
 * @param port  the port of the client
 * @return the client corrosponding to the port, or NULL if no such client exists
 */
Client *Client::get_client(int port) {
  map<int , Client *>::iterator iter;

  iter = cli_map.find(port);
  if (iter != cli_map.end()) {
     return iter->second;
     }

  return NULL;
}

/*
 * Lookup the client or create if needed
 *
 * @param port  the port of the client
 * @return the client corrosponding to the port, or NULL if an error occurs
 */
Client *Client::get_client_or_create(int port) {
  Client *cli = get_client(port);

  if(cli == NULL) {
    cli = new Client(port);
    pair<int , Client*> p (port, cli);
    cli_map.insert(p);
  }

  // this could still be NULL
  return cli;
}

/*
 * Cleanup all clients
 *
 * @return 0 on sucess, non 0 on error
 */
int Client::clean_up() {
  map<int, Client*>::iterator iter;

  //unload all plugins
  for(iter = cli_map.begin(); iter != cli_map.end(); ) {
    // increment the iter here
    // the universe will remove itself from the map
    // and invalidate the iter
    delete (*iter++).second;
  }

  cli_map.clear();

  return 0;
}
