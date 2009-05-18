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
 * shownet.c
 * Part of libshownet
 * Copyright (C) 2005 Simon Newton
 */

// required for vasprintf
// patches welcomed to remove it :)
#define _GNU_SOURCE

#include "private.h"
#include <stdio.h>
#include <stdarg.h>

char *shownet_errstr = NULL;

static int is_shownet(shownet_packet_t *p);
static int send_dmx(node n, uint8_t universe, int length, uint8_t *data);
static int handle(node n, shownet_packet p);
static int handle_dmx(node n, shownet_packet p);


/*
 * libshownet error function
 *
 * This writes the error string to shownet_strerror, which can be accessed
 * using shownet_strerror();
 *
 * Is vasprintf available on OSX ?
 */
void shownet_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  free(shownet_errstr);
  vasprintf(&shownet_errstr, fmt , ap) ,
  va_end(ap);
}


/*
 * Fetches the shownet error string
 *
 * @return the error string
 */
const char *shownet_strerror() {
  return shownet_errstr;
}

/**
 * Creates a new ShowNet node.
 * Takes a string containing the ip address to bind to, if the string is NULL
 * it uses the first non loopback address
 *
 * @param ip the IP address to bind to
 * @param debug level of logging provided 0: none
 * @return an shownet_node
 */
shownet_node shownet_new(const char *ip, int debug) {
  node n;
  int ret;

  n = malloc(sizeof(shownet_node_t ) );

  if (!n) {
    shownet_error("malloc failure");
    return NULL;
  }

  memset(n, 0x0, sizeof(shownet_node_t));

  // default parameters
  n->state.mode = SHOWNET_OFF;
  n->state.verbose = debug;
  n->state.packet_count = 0;

  ret = shownet_net_init(n, ip);

  if (ret) {
    free(n);
    return NULL;
  }

  return (shownet_node) n;
}


/**
 * Starts the ShowNet node.
 * Binds to the network sockets
 *
 * @param vn the shownet_node
 * @return 0 on success, non 0 on failure
 */
int shownet_start(shownet_node vn) {
  node n = (node) vn;
  int ret;

  return_if_null(vn);
  return_if_on(n);

  if ((ret = shownet_net_start(n)))
    return ret;

  n->state.mode = SHOWNET_ON;
  return SHOWNET_EOK;
}


/**
 * Stops the ShowNet node
 * Closes the network sockets held by the node
 *
 * @param vn the shownet_node
 * @return 0 on success, non 0 on failure
 */
int shownet_stop(shownet_node vn) {
  node n = (node) vn;
  int ret;

  return_if_null(vn);
  return_if_off(n);

  if ((ret = shownet_net_close(n)))
    return ret;

  n->state.mode = SHOWNET_OFF;
  return SHOWNET_EOK;
}


/**
 * Free the memory associated with this node
 *
 * @param vn the shownet_node
 */
int shownet_destroy(shownet_node vn) {
  return_if_null(vn);
  free(vn);
  return SHOWNET_EOK;
}


/**
 * Handle any received packets.
 * This function is the workhorse of libshownet. You have a couple of options:
 *   - use shownet_get_sd() to retrieve the socket descriptors and select to
 *     detect network activity. Then call shownet_read(node,0)
 *     when activity is detected.
 *   - call shownet_read repeatedly from within a loop with an appropriate
 *     timeout
 *
 * @param vn the shownet_node
 * @param timeout the number of seconds to block for if nothing is pending
 * @return 0 on success, -1 on failure
 */
int shownet_read(shownet_node vn, int timeout) {
  node n = (node) vn;
  shownet_packet_t p;
  int ret;

  return_if_null(vn);
  return_if_off(n);

  while (1) {
    memset(&p.data, 0x0, sizeof(p.data));

    if ((ret = shownet_net_recv(n, &p, timeout)) < 0)
      return ret;

    // nothing to read
    if (ret == RECV_NO_DATA)
      break;

    // skip this packet (filtered)
    if (p.length == 0)
      continue;

    if (is_shownet(&p)) {
      // should we be returning errors here?
      handle(n, &p);
    }
  }
  return SHOWNET_EOK;
}


/*
 * This is a special callback which is invoked when dmx data is received.
 * Unlike libartnet we pass the data back here, an esp node can recv on any
 * of 256 channels, we don't want to maintain buffer for them.
 *
 * @param vn     The shownet_node
 * @param fh    The callback to invoke (parameters passwd are the shownet_node, the port_id
 *           that received the dmx, and some user data
 * @param data    Data to be passed to the handler when its called
 *
 */
int shownet_set_dmx_handler(shownet_node vn,
    int (*fh)(shownet_node n, uint8_t universe, int length, uint8_t *data,
              void *d),
    void *data) {
  node n = (node) vn;
  return_if_null(vn);

  n->dmx_callback.fh = fh;
  n->dmx_callback.data = data;
  return SHOWNET_EOK;
}


/*
 * Sends some dmx data
 *
 * @param vn the shownet_node
 * @param universe universe to address it to
 * @param length length of data
 * @param data pointer to dmx data
 *
 */
int shownet_send_dmx(shownet_node vn, uint8_t universe, int16_t length,
                     uint8_t *data ) {
  node n = (node) vn;
  return_if_null(vn);
  return_if_off(n);

  if (length < 1 || length > SHOWNET_DMX_LENGTH) {
    shownet_error("%s : Length of dmx data out of bounds (should be between"
                  "1 and 512, was %i)", __FUNCTION__ , length);
    return SHOWNET_EARG;
  }

  if (universe >= SHOWNET_MAX_UNIVERSES) {
    shownet_error("%s : Universe index out of bounds (should be between 0 and"
                  "%d, was %d)", __FUNCTION__ , SHOWNET_MAX_UNIVERSES,
                  universe);
    return SHOWNET_EARG;
  }

  return send_dmx(n, universe, length, data);
}


/**
 * Sets the name of the node.
 * The string should be null terminated and a maxmium of 18 characters will be used
 *
 * @param vn the shownet_node
 * @param name the short name of the node.
 */
int shownet_set_name(shownet_node vn, const char *name) {
  node n = (node) vn;
  return_if_null(vn);

  strncpy((char *) &n->state.name, name, SHOWNET_NAME_LENGTH);
  return SHOWNET_EOK;
}


/**
 * Returns the socket descriptor associated with this shownet_node.
 * libshownet now uses a single descriptor per node  bound to the wildcard
 * address
 *
 * @param vn the shownet_node
 * @param socket the index of the socket descriptor to fetch (0 or 1)
 * @return the socket descriptor, or < 0 on error
 */
int shownet_get_sd(shownet_node vn) {
  node n = (node) vn;
  return_if_null(vn);
  return_if_off(n);

  return n->sd;
}



//-----------------------------------------------------------------------------
// Private functions follow

/*
 * Is this a shownet datagram?
 *
 * @param p   the shownet datagram
 * @return true if it is, false otherwise
 */
static int is_shownet(shownet_packet_t *p) {
  if (p->data.dmx.sigHi == 0x80 && p->data.dmx.sigLo == 0x8f)
    return 1;
  else
    return 0;
}


/*
 * Encode the dmx for shownet
 *
 * @return the length used in the dst array
 */
static int encode_dmx(uint8_t *src, int slen, uint8_t *dst, int dlen) {
  int i, j, l;
  int di = 0;

  for (i=0; i < slen && di < dlen;) {

    // look ahead to see if this is a repeating value
    j = i+1;
    while (j < slen && src[i] == src[j] && j-i < 0x7f ) {
      j++;
    }

    // if the number of repeats is more than 2
    // don't encode only two repeats,
    if (j-i > 2) {
      // if room left in dst buffer
      if (dlen -di > 1) {
        dst[di++] = (REPEAT_FLAG | (j-i));
        dst[di++] = src[i];
      } else {
        // else return what we have done so far
        return di;
      }
      i = j;

    } else {
      // this value doesn't repeat more than twice
      // find out where the next repeat starts

      // postcondition: j is one more than the last value we want to send
      for (j = i+1; j < slen - 2 && j-i < 0x7f; j++) {
        // at the end  of the array
        if (j == slen - 2) {
          j = slen;
          break;
        }

        // if we're found a repeat of 3 or more stop here
        if ( src[j] == src[j+1] && src[j+1] == src[j+2] )
          break;

      }

       // if we have enough room left for all the values
      if (di+j-i < dlen) {
        dst[di++] = j-i;
        memcpy(&dst[di], &src[i], j-i);
        di += j-i;
        i += j-i;

      // see how much data we can get in
      } else if (dlen - di > 1) {
        l = dlen - di -1;
        dst[di++] = l;
        memcpy(&dst[di], &src[i], l);
        di += l;
        return di;

      } else {
        return di;
      }
    }
  }
  return di;
}


/*
 * decode run-length encoded ShowNet data
 */
static int decode_rle(uint8_t *src, int slen, uint8_t *dst, int dlen) {
  int i,l;
  int lr = dlen;
  int di = 0;

  for (i=0; i < slen && lr > 0; ) {
    l = src[i] & (~REPEAT_FLAG);
    l = min(lr, l);

    if (src[i] & REPEAT_FLAG) {
      i++;
      memset(&dst[di], src[i++], min(l, lr));
      di += l;
    } else {
      memcpy(&dst[di], &src[++i], min(l, lr));
      di += l;
      i += l;
    }
    lr -= l;
  }
  return di;
}


/*
 * Send some dmx data
 *
 */
static int send_dmx(node n, uint8_t universe, int length, uint8_t *data) {
  shownet_packet_t p;
  int ret, len, enc_len;

  len = min(length, SHOWNET_DMX_LENGTH);

  memset(&p.data, 0x00, sizeof(p.data));

  // set dst addr and length
  p.to.s_addr = n->state.bcast_addr.s_addr;
  p.length = sizeof(shownet_data_t);

  // setup the fields in the datagram
  p.data.dmx.sigHi = 0x80;
  p.data.dmx.sigLo = 0x8f;

  // set ip
  memcpy(p.data.dmx.ip, &n->state.ip_addr.s_addr,4);

  p.data.dmx.netSlot[0] = (universe * 0x0200) + 0x01;
  p.data.dmx.netSlot[1] = 0;
  p.data.dmx.netSlot[2] = 0;
  p.data.dmx.netSlot[3] = 0;
  p.data.dmx.slotSize[0] = len;
  p.data.dmx.slotSize[1] = 0;
  p.data.dmx.slotSize[2] = 0;
  p.data.dmx.slotSize[3] = 0;

  enc_len = encode_dmx(data,len, p.data.dmx.data, SHOWNET_DMX_LENGTH);

  p.data.dmx.indexBlock[0] = 0x0b;
  p.data.dmx.indexBlock[1] = 0x0b + enc_len;
  p.data.dmx.indexBlock[2] = 0;
  p.data.dmx.indexBlock[3] = 0;
  p.data.dmx.indexBlock[4] = 0;

  p.data.dmx.packetCountHi = short_gethi(n->state.packet_count);
  p.data.dmx.packetCountLo = short_getlo(n->state.packet_count);
  // magic numbers - not sure what these do
  p.data.dmx.block[2] = 0x58;
  p.data.dmx.block[3] = 0x02;
  strncpy((char*) p.data.dmx.name, n->state.name, SHOWNET_NAME_LENGTH);

  ret = shownet_net_send(n, &p);

  if (!ret)
    n->state.packet_count++;

  return ret;
}


/**
 * handle a shownet dmx packet
 *
 */
static int handle_dmx(node n, shownet_packet p) {
  int len, enc_len, slot_len, start, uni;
  uint8_t dmx[SHOWNET_DMX_LENGTH];

  memset(dmx, 0x00, SHOWNET_DMX_LENGTH);
  if (n->dmx_callback.fh == NULL)
    return 0;

  // TODO: this is a mega hack
  // fix it when we have access to the strand again
  // hmm seems to work... :)
  enc_len = p->data.dmx.indexBlock[1] - p->data.dmx.indexBlock[0];
  slot_len = p->data.dmx.slotSize[0];
  uni = p->data.dmx.netSlot[0] / SHOWNET_DMX_LENGTH;

  start = (p->data.dmx.netSlot[0] % SHOWNET_DMX_LENGTH) - 1;

  if (slot_len != enc_len) {
    len = decode_rle(p->data.dmx.data, enc_len,
        &dmx[start], sizeof dmx - start);
  } else {
    len = min(enc_len, sizeof dmx - start);
    memcpy(&dmx[start], p->data.dmx.data, len);
  }

  n->dmx_callback.fh(n, uni, start + len, dmx, n->dmx_callback.data);
  return 0;
}


/**
 * the main handler for an shownet packet. calls
 * the appropriate handler function
 *
 */
static int handle(node n, shownet_packet p) {
  if (p->data.dmx.sigHi == 0x80 && p->data.dmx.sigLo == 0x8f) {
      return handle_dmx(n,p);
  }
  return SHOWNET_EOK;
}
