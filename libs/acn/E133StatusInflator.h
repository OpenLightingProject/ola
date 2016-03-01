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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * E133StatusInflator.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef LIBS_ACN_E133STATUSINFLATOR_H_
#define LIBS_ACN_E133STATUSINFLATOR_H_

#include <memory>
#include <string>
#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "ola/e133/E133Enums.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/TransportHeader.h"
#include "libs/acn/E133Header.h"

namespace ola {
namespace acn {

class E133StatusInflator: public BaseInflator {
 public:
    // These are pointers so the callers don't have to pull in all the headers.
    typedef ola::Callback4<void,
                           const TransportHeader*,  // src ip & port
                           const E133Header*,  // the E1.33 header
                           uint16_t,  // the E1.33 Status code
                           const std::string&  // rdm data
                          > StatusMessageHandler;

    E133StatusInflator();

    uint32_t Id() const { return ola::acn::VECTOR_FRAMING_STATUS; }

    // Ownership is transferred.
    void SetStatusHandler(StatusMessageHandler *handler) {
      m_handler.reset(handler);
    }

 protected:
    // The 'header' is 0 bytes in length.
    bool DecodeHeader(HeaderSet*,
                      const uint8_t*,
                      unsigned int,
                      unsigned int *bytes_used) {
      *bytes_used = 0;
      return true;
    }

    void ResetHeaderField() {}  // namespace noop

    virtual bool HandlePDUData(uint32_t vector,
                               const HeaderSet &headers,
                               const uint8_t *data,
                               unsigned int pdu_len);

 private:
    std::auto_ptr<StatusMessageHandler> m_handler;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_E133STATUSINFLATOR_H_
