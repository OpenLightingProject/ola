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
 * LLRPProbeReplyInflator.h
 * Copyright (C) 2020 Peter Newman
 */

#ifndef LIBS_ACN_LLRPPROBEREPLYINFLATOR_H_
#define LIBS_ACN_LLRPPROBEREPLYINFLATOR_H_

#include "ola/Callback.h"
#include "ola/acn/ACNVectors.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/HeaderSet.h"
#include "libs/acn/LLRPProbeReplyPDU.h"

namespace ola {
namespace acn {

class LLRPProbeReplyInflator: public BaseInflator {
  friend class LLRPProbeReplyInflatorTest;

 public:
  struct LLRPProbeReply {
    explicit LLRPProbeReply(const ola::rdm::UID &_uid)
       : uid(_uid) {
    }
    ola::rdm::UID uid;
    ola::network::MACAddress hardware_address;
    ola::acn::LLRPProbeReplyPDU::LLRPComponentType component_type;
  };


  // These are pointers so the callers don't have to pull in all the headers.
  typedef ola::Callback2<void,
                         const HeaderSet*,  // the HeaderSet
                         const LLRPProbeReply&  // Probe Reply Data
                        > LLRPProbeReplyHandler;

  LLRPProbeReplyInflator();
  ~LLRPProbeReplyInflator() {}

  uint32_t Id() const { return ola::acn::VECTOR_LLRP_PROBE_REPLY; }

  void SetLLRPProbeReplyHandler(LLRPProbeReplyHandler *handler);

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
  std::unique_ptr<LLRPProbeReplyHandler> m_llrp_probe_reply_handler;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_LLRPPROBEREPLYINFLATOR_H_
