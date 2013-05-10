/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RDMInflator.h
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_RDMINFLATOR_H_
#define PLUGINS_E131_E131_RDMINFLATOR_H_

#include <string>
#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/TransportHeader.h"
#include "plugins/e131/e131/E133Header.h"

namespace ola {
namespace plugin {
namespace e131 {

class RDMInflator: public BaseInflator {
  friend class RDMInflatorTest;

  public:
    // These are pointers so the callers don't have to pull in al the headers.
    typedef ola::Callback3<void,
                           const TransportHeader*,  // src ip & port
                           const E133Header*,  // the E1.33 header
                           const std::string&  // rdm data
                          > RDMMessageHandler;

    explicit RDMInflator();
    ~RDMInflator() {}

    uint32_t Id() const { return ola::acn::VECTOR_FRAMING_RDMNET; }

    void SetRDMHandler(RDMMessageHandler *handler);

    static const unsigned int VECTOR_RDMNET_DATA = 0xcc;

  protected:
    bool DecodeHeader(HeaderSet &headers,
                      const uint8_t *data,
                      unsigned int len,
                      unsigned int &bytes_used);

    void ResetHeaderField() {}  // namespace noop

    virtual bool HandlePDUData(uint32_t vector,
                               HeaderSet &headers,
                               const uint8_t *data,
                               unsigned int pdu_len);

  private:
    std::auto_ptr<RDMMessageHandler> m_rdm_handler;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_RDMINFLATOR_H_
