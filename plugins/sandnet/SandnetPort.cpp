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
 *
 * sandnetport.cpp
 * The SandNet plugin for lla
 * Copyright (C) 2005  Simon Newton
 */

#include "SandnetPort.h"
#include "SandnetDevice.h"

#include <llad/universe.h>
#include <llad/logger.h>

#include <string.h>

#define min(a,b) a<b?a:b

SandNetPort::SandNetPort(Device *parent, int id) :
  Port(parent, id),
  m_buf(NULL),
  m_len(DMX_LENGTH) {

}

SandNetPort::~SandNetPort() {

  if (can_read())
    free(m_buf);
}

int SandNetPort::can_read() const {
  // ports 2 to 9 are input
  return ( get_id()>= SANDNET_MAX_PORTS && get_id() < SANDNET_MAX_PORTS + INPUT_PORTS);
}


int SandNetPort::can_write() const {
  // ports 0 & 1 are output (sandnet allows 2 ports per device)
  return ( get_id() >= 0 && get_id() < SANDNET_MAX_PORTS);
}

/*
 * Write operation
 *
 * @param  data  pointer to the dmx data
 * @param  length  the length of the data
 *
 */
int SandNetPort::write(uint8_t *data, unsigned int length) {
  SandNetDevice *dev = (SandNetDevice*) get_device();

  if (!can_write())
    return -1;

  if (sandnet_send_dmx(dev->get_node(), get_id(), length, data)) {
    Logger::instance()->log(Logger::WARN, "SandnetPlugin: sandnet_send_dmx failed %s", sandnet_strerror() );
    return -1;
  }
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
int SandNetPort::read(uint8_t *data, unsigned int length) {
  unsigned int len;

  if (!can_read())
    return -1;

  len = min(m_len, length);
  memcpy(data, m_buf, len);
  return len;
}

/*
 * Update the data buffer for this port
 *
 */
int SandNetPort::update_buffer(uint8_t *data, int length) {
  int len = min(DMX_LENGTH, length);

  // we can't update if this isn't a input port
  if (!can_read())
    return -1;

  if (m_buf == NULL) {
    m_buf = (uint8_t*) malloc(m_len);

    if (m_buf == NULL) {
      Logger::instance()->log(Logger::CRIT, "SandnetPlugin: malloc failed");
      return -1;
    } else
      memset(m_buf, 0x00, m_len);
  }

  Logger::instance()->log(Logger::DEBUG, "SandNet: Updating dmx buffer for port %d", length);
  memcpy(m_buf, data, len);
  dmx_changed();
  return 0;
}


/*
 * We override the set universe method to update the universe -> port hash
 */
int SandNetPort::set_universe(Universe *uni) {
  SandNetDevice *dev = (SandNetDevice*) get_device();

  // if we're unpatching remove the old universe mapping
  if ( uni == NULL && get_universe() != NULL) {
    dev->port_map(get_universe(), NULL);
  } else {
    // new patch
    dev->port_map(uni, this);
  }

  // call the super method
  return Port::set_universe(uni);
}
