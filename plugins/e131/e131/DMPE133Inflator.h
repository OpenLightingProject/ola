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
 * DMPE133Inflator.h
 * This is a subclass of the DMPInflator which knows how to handle DMP over
 * E1.33 messages.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_DMPE133INFLATOR_H_
#define PLUGINS_E131_E131_DMPE133INFLATOR_H_

#include <map>
#include "ola/Clock.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMCommand.h"
#include "plugins/e131/e131/DMPInflator.h"
#include "plugins/e131/e131/E133Layer.h"

namespace ola {
namespace plugin {
namespace e131 {

class DMPE133Inflator: public DMPInflator {
  friend class DMPE133InflatorTest;

  public:
    DMPE133Inflator(E133Layer *e133_layer)
        : DMPInflator(),
          m_e133_layer(e133_layer) {
    }
    ~DMPE133Inflator();

    typedef ola::Callback2<void,
                           unsigned int,
                           const ola::rdm::RDMRequest*> RDMHandler;
    typedef ola::Callback1<void,
                           const ola::rdm::RDMRequest*> ManagementRDMHandler;

    bool SetRDMHandler(unsigned int universe, RDMHandler *handler);
    bool RemoveRDMHandler(unsigned int universe);

    void SetRDMManagementhandler(ManagementRDMHandler *handler);
    void RemoveRDMManagementHandler();

  protected:
    virtual bool HandlePDUData(uint32_t vector,
                               HeaderSet &headers,
                               const uint8_t *data,
                               unsigned int pdu_len);

  private:
    typedef std::map<unsigned int, RDMHandler*> universe_handler_map;
    universe_handler_map m_rdm_handlers;
    ManagementRDMHandler *m_rdm_management_handler;

    E133Layer *m_e133_layer;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_DMPE133INFLATOR_H_
