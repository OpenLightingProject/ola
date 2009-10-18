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
 * DMPE131Inflator.h
 * This is a subclass of the DMPInflator which knows how to handle DMP over
 * E1.31 messages.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef OLA_DMP_DMPE131INFLATOR_H
#define OLA_DMP_DMPE131INFLATOR_H

#include <map>
#include <ola/Closure.h>
#include <ola/DmxBuffer.h>

#include "DMPInflator.h"
#include "E131Layer.h"

namespace ola {
namespace e131 {

class DMPE131Inflator: public DMPInflator {
  friend class DMPE131InflatorTest;

  public:
    DMPE131Inflator(E131Layer *e131_layer):
      DMPInflator(),
      m_e131_layer(e131_layer) {}
    ~DMPE131Inflator();


    bool SetHandler(unsigned int universe, ola::DmxBuffer *buffer,
                    ola::Closure *handler);
    bool RemoveHandler(unsigned int universe);

  protected:
    virtual bool HandlePDUData(uint32_t vector, HeaderSet &headers,
                               const uint8_t *data, unsigned int pdu_len);

  private:
    typedef struct {
      ola::DmxBuffer *buffer;
      Closure *closure;
    } universe_handler;

    std::map<unsigned int, universe_handler> m_handlers;
    E131Layer *m_e131_layer;
};

} // e131
} // ola

#endif
