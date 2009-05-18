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
 * shownet.h
 * Interface to libshownet
 * Copyright (C) 2005 Simon Newton
 */

/** @file shownet/shownet.h */

#ifndef _defined_shownet_h
#define _defined_shownet_h

#include <stdint.h>
// order is important here for osx
#include <sys/types.h>
#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

// error codes
#define SHOWNET_EOK  0
#define SHOWNET_ENET -1      // network error
#define SHOWNET_EMEM -2      // memory error
#define SHOWNET_EARG -3      // argument error
#define SHOWNET_ESTATE -4    // state error
#define SHOWNET_EACTION -5   // invalid action

#define SHOWNET_MAX_UNIVERSES 8
/* The ShowNet node */
typedef void *shownet_node;

extern const char *shownet_strerror();

// node control functions
extern shownet_node shownet_new(const char *ip, int verbose);
extern int shownet_start(shownet_node n);
extern int shownet_read(shownet_node n, int timeout);
extern int shownet_stop(shownet_node n);
int shownet_destroy(shownet_node vn);


// handler functions
extern int shownet_set_dmx_handler(shownet_node vn, int (*fh)(shownet_node n, uint8_t universe, int length, uint8_t *data,  void *d), void *data );

// send functions
extern int shownet_send_dmx(shownet_node n, uint8_t universe, int16_t length, uint8_t *data);


// state changing functions
extern int shownet_set_name(shownet_node vn, const char *name);

// misc
extern int shownet_get_sd(shownet_node n);

#ifdef __cplusplus
}
#endif

#endif
