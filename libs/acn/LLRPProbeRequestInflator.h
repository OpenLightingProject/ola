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
 * LLRPProbeRequestInflator.h
 * Copyright (C) 2020 Peter Newman
 */

#ifndef LIBS_ACN_LLRPPROBEREQUESTINFLATOR_H_
#define LIBS_ACN_LLRPPROBEREQUESTINFLATOR_H_

#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/HeaderSet.h"

namespace ola {
namespace acn {

class LLRPProbeRequestInflator: public BaseInflator {
  friend class LLRPProbeRequestInflatorTest;

 public:
  struct LLRPProbeRequest {
    LLRPProbeRequest(const ola::rdm::UID &_lower, const ola::rdm::UID &_upper)
       : lower(_lower),
         upper(_upper) {
    }
    ola::rdm::UID lower;
    ola::rdm::UID upper;
    bool client_tcp_connection_inactive;
    bool brokers_only;
    ola::rdm::UIDSet known_uids;
  };


  // These are pointers so the callers don't have to pull in all the headers.
  typedef ola::Callback2<void,
                         const HeaderSet*,  // the HeaderSet
                         const LLRPProbeRequest&  // Probe Request Data
                        > LLRPProbeRequestHandler;

  LLRPProbeRequestInflator();
  ~LLRPProbeRequestInflator() {}

  uint32_t Id() const { return ola::acn::VECTOR_LLRP_PROBE_REQUEST; }

  void SetLLRPProbeRequestHandler(LLRPProbeRequestHandler *handler);

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
  std::auto_ptr<LLRPProbeRequestHandler> m_llrp_probe_request_handler;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_LLRPPROBEREQUESTINFLATOR_H_
