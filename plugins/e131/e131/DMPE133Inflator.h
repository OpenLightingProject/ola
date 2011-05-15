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
#include <string>
#include "ola/Callback.h"
#include "plugins/e131/e131/DMPInflator.h"
#include "plugins/e131/e131/E133Header.h"
#include "ola/network/IPV4Address.h"

namespace ola {
namespace plugin {
namespace e131 {

class DMPE133Inflator: public DMPInflator {
  friend class DMPE133InflatorTest;

  public:
    explicit DMPE133Inflator(class E133Layer *e133_layer);
    ~DMPE133Inflator();

    typedef ola::Callback3<void,
                           const ola::network::IPV4Address&,  // src ip
                           const E133Header&,  // the E1.33 header
                           const std::string&  // rdm data
                          > RDMMessageHandler;

    bool SetRDMHandler(unsigned int universe, RDMMessageHandler *handler);
    bool RemoveRDMHandler(unsigned int universe);

    bool SetRDMManagementhandler(RDMMessageHandler *handler);
    void RemoveRDMManagementHandler();

  protected:
    virtual bool HandlePDUData(uint32_t vector,
                               HeaderSet &headers,
                               const uint8_t *data,
                               unsigned int pdu_len);

  private:
    typedef std::map<unsigned int, RDMMessageHandler*> universe_handler_map;
    universe_handler_map m_rdm_handlers;
    RDMMessageHandler *m_management_handler;
    class E133Layer *m_e133_layer;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_DMPE133INFLATOR_H_
