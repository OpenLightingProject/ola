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
 * RPTStatusInflator.h
 * Interface for the RPTStatusInflator class.
 * Copyright (C) 2024 Peter Newman
 */

#ifndef LIBS_ACN_RPTSTATUSINFLATOR_H_
#define LIBS_ACN_RPTSTATUSINFLATOR_H_

#include <string>
#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/TransportHeader.h"
#include "libs/acn/E133Header.h"

namespace ola {
namespace acn {

class RPTStatusInflator: public BaseInflator {
  friend class RPTStatusInflatorTest;

 public:
  // These are pointers so the callers don't have to pull in all the headers.
  // TODO(Peter): Should this be the E133Header or the RPTHeader?
  typedef ola::Callback4<void,
                         const TransportHeader*,  // src ip & port
                         const E133Header*,  // the E1.33 header
                         const RPTStatusVector&,  // the status vector
                         const std::string&  // status message
                        > RPTStatusHandler;

  RPTStatusInflator();
  ~RPTStatusInflator() {}

  uint32_t Id() const { return ola::acn::VECTOR_RPT_STATUS; }

  void SetRPTStatusHandler(RPTStatusHandler *handler);

 protected:
  bool DecodeHeader(HeaderSet *headers,
                    const uint8_t *data,
                    unsigned int len,
                    unsigned int *bytes_used);

  void ResetHeaderField() {}  // namespace noop

  virtual bool HandlePDUData(uint32_t vector,
                             const HeaderSet &headers,
                             const uint8_t *data,
                             unsigned int pdu_len);

 private:
  std::auto_ptr<RPTStatusHandler> m_rpt_status_handler;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_RPTSTATUSINFLATOR_H_
