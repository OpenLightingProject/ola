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
 * E131Port.cpp
 * The E1.31 plugin for ola
 * Copyright (C) 2007 Simon Newton
 */

#include <string.h>

#include <olad/universe.h>

#include <acn/E131DmpLayer.h>
#include "E131Port.h"
#include "E131Device.h"

#define min(a,b) a<b?a:b


int E131Port::can_read() const {
  // ports 0 to 3 are input
  return ( get_id() >= 0 && get_id() < NUMB_PORTS);
}

int E131Port::can_write() const {
  // ports 4 to 7 are output
  return ( get_id() >= NUMB_PORTS && get_id() < 2 * NUMB_PORTS);
}


/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 */
int E131Port::write(uint8_t *data, unsigned int length) {

  if (!can_write())
    return -1;

  if (get_universe())
    m_layer->send(get_universe()->get_uid(), data, length);

  return 0;
}


/*
 * Read operation
 *
 * @param   data  buffer to read data into
 * @param   length  length of data to read
 *
 * @return  the amount of data read
 */
int E131Port::read(uint8_t *data, unsigned int length) {
  unsigned int l = length < m_len ? length : m_len;

  memcpy(data, m_data, l);
  return l;
}


/*
 * override this so we can set the callback
 */
int E131Port::set_universe(Universe *uni) {

  if (get_universe())
    m_layer->unregister_uni(get_universe()->get_uid());

  Port::set_universe(uni);

  if (can_read() && uni)
   m_layer->register_uni(uni->get_uid(), data_callback, (void*) this);
  return 0;
}



void E131Port::new_data(const uint8_t *data, unsigned int len) {
  int l = DMX_LENGTH < len ? DMX_LENGTH : len;
  memcpy(m_data, data, l);
  m_len = l;
  dmx_changed();
}

/*
 * Static method for the E131DmpLayer to call
 */
void E131Port::data_callback(const uint8_t *dmx, unsigned int len, void *data) {
  E131Port *prt = (E131Port*) data;
  prt->new_data(dmx, len);
}
