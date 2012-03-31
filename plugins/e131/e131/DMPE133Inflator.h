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
#include "plugins/e131/e131/TransportHeader.h"
#include "plugins/e131/e131/E133Header.h"

namespace ola {
namespace plugin {
namespace e131 {

class DMPE133Inflator: public DMPInflator {
  friend class DMPE133InflatorTest;

  public:
    DMPE133Inflator(
        ola::Callback1<void, const TransportHeader&> *on_data = NULL);
    ~DMPE133Inflator();

    typedef ola::Callback3<void,
                           const TransportHeader&,  // src ip & port
                           const E133Header&,  // the E1.33 header
                           const std::string&  // rdm data
                          > RDMMessageHandler;

    bool SetRDMHandler(uint16_t endpoint, RDMMessageHandler *handler);
    bool RemoveRDMHandler(uint16_t endpoint);

  protected:
    virtual bool HandlePDUData(uint32_t vector,
                               HeaderSet &headers,
                               const uint8_t *data,
                               unsigned int pdu_len);

  private:
    typedef std::map<uint16_t, RDMMessageHandler*> endpoint_handler_map;
    endpoint_handler_map m_rdm_handlers;
    ola::Callback1<void, const TransportHeader&> *m_on_data;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_DMPE133INFLATOR_H_
