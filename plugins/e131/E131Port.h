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
 * E131Port.h
 * The E1.31 plugin for lla
 * Copyright (C) 2007 Simon Newton
 */

#ifndef E131PORT_H
#define E131PORT_H

#include <llad/port.h>

class E131Port : public Port  {

  public:
    E131Port(Device *parent, int id, class E131DmpLayer *l) : Port(parent, id),
             m_layer(l) {};

    int set_universe(Universe *uni) ;
    int write(uint8_t *data, unsigned int length);
    int read(uint8_t *data, unsigned int length);

    int can_read() const;
    int can_write() const ;

    static const int NUMB_PORTS = 5;
    static void data_callback(const uint8_t *dmx, unsigned int len, void *data);
  private:
    void new_data(const uint8_t *data, unsigned int len);

    static const unsigned int DMX_LENGTH = 512;
    class E131DmpLayer *m_layer;
    uint8_t m_data[DMX_LENGTH];
    unsigned int m_len;
};

#endif
