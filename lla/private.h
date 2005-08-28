/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * private.h
 * Defines private data structures used by liblla
 * Copyright (C) 2005  Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif


#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))

// this should prob be somewhere else									
#define MAX_DMX 512

// the number of DMX buffers we hold (number of simultaneous universes we can use)
#define MAX_BUF	10
	
#define LLAD_ADDR "127.0.0.1"		// address to bind to
#define LLAD_PORT 8898				// port to listen on

// check if the connection is null and return an error
#define return_if_null(con) if (con == NULL) { \
	return -1 ; \
}

/*
 * the dmx callback is triggered when a dmx packet arrives
 */
typedef struct {
	int (*fh)(lla_con c, int uni, void *data) ;
	void *data;	
} dmx_callback_t ;

/*
 * our lla connection
 *
 */

// we need to allocate space for dmx buffers here
// and we prob want a uni -> buffer mapping
typedef struct {
	int sd;
	int connected ;
	dmx_callback_t dmx_c ;
	int	buf_map[MAX_BUF] ;
	uint8_t buf[MAX_BUF][MAX_DMX] ;
	lla_device *devices;
	lla_plugin *plugins;
	lla_universe *universes;
	char *desc ;
} lla_connection;



