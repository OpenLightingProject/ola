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
 * Private definitions, data structures, macros and functions for libshownet
 * Copyright (C) 2005 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <shownet.h>
#include <shownet/packets.h>

#ifndef _defs_defined
#define _defs_defined

// port to listen and send from, shownet seems to use 2510 as well
static const int SHOWNET_PORT = 2501;
static const int RECV_NO_DATA = 1;
static const uint8_t REPEAT_FLAG = 0x80;

// non shownet specific
static const int TRUE = 1;
static const int FALSE = 0;

#define SA struct sockaddr
#define SI struct in_addr

#define min(a,b)    ((a) < (b) ? (a) : (b))
#define max(a,b)    ((a) > (b) ? (a) : (b))

// byte ordering macros
#define bswap_16(x)  ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

//#define bytes_to_short(x,y) ( (x << 8) & 0xff00 | (l & 0x00FF) );

// htols : convert short from host to little endian order
// ltohs : convert short from little endian to host order
#ifdef HAVE_ENDIAN_H
# if BYTE_ORDER == __BIG_ENDIAN
#  define htols(x)   bswap_16 (x)
# else
#  define htols(x)  (x)
# endif
#else
# if BYTE_ORDER == BIG_ENDIAN
#  define htols(x)   bswap_16 (x)
# else
#  define htols(x)  (x)
# endif
#endif

#define short_gethi(x) ((0xff00 & x) >> 8)
#define short_getlo(x)  (0x00ff & x)


#define shownet_error_malloc() shownet_error("%s : malloc error %s" , __FUNCTION__, strerror(errno) )
#define shownet_error_realloc() shownet_error("%s : realloc error %s" , __FUNCTION__, strerror(errno) )
#define shownet_error_nullnode() shownet_error("%s : argument 1 (shownet_node) was null" , __FUNCTION__ )

// check if the node is null and return an error
#define return_if_null(node) if (node == NULL) { \
  shownet_error_nullnode(); \
  return SHOWNET_EARG; \
}

// return if the node is off
#define return_if_off(node) if (node->state.mode == SHOWNET_OFF) { \
  shownet_error("%s : node is not on" , __FUNCTION__ ); \
  return SHOWNET_ESTATE; \
}

// return if the node is on
#define return_if_on(node) if (node->state.mode == SHOWNET_ON) { \
  shownet_error("%s : node is not on" , __FUNCTION__ ); \
  return SHOWNET_ESTATE; \
}

/*
 * the dmx callback is triggered when a dmx packet arrives
 */
typedef struct {
  int (*fh)(shownet_node n, uint8_t universe, int length, uint8_t *data, void *d);
  void *data;
} dmx_callback_t;


// the status of the node
typedef enum {
  SHOWNET_OFF,
  SHOWNET_ON
} node_status_t;

// struct to hold the state of the node
typedef struct {
  node_status_t mode;
  SI ip_addr;
  SI bcast_addr;
  char name[SHOWNET_NAME_LENGTH];
  uint16_t packet_count;
  int verbose;
} node_state_t;


/**
 * The main node structure
 */
typedef struct shownet_node_s{
  int sd;                      // the two sockets
  node_state_t state;          // the state struct
  dmx_callback_t dmx_callback; // the callbacks struct
} shownet_node_t;

typedef shownet_node_t *node;

// Global variables
extern char *shownet_errstr;

// Function definitions follow

// shownet.c
void shownet_error(const char *fmt, ...);

// exported from network.c
ssize_t shownet_net_recv(node n, shownet_packet p, int block);
int shownet_net_send(node n, shownet_packet p);
int shownet_net_init(node n, const char *ip);
int shownet_net_start(node n);
int shownet_net_close(node n);

#endif
