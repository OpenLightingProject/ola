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
 * BrokerClientEntryHeader.h
 * The E1.33 Broker Client Entry Header
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_BROKERCLIENTENTRYHEADER_H_
#define LIBS_ACN_BROKERCLIENTENTRYHEADER_H_

#include <ola/acn/CID.h>
#include <ola/base/Macro.h>

#include <stdint.h>

namespace ola {
namespace acn {

// TODO(Peter): I think technically this probably shouldn't be a header and
// instead is just data at this level!
/*
 * Header for the Broker Client Entry level
 */
class BrokerClientEntryHeader {
 public:
    BrokerClientEntryHeader() {}

    BrokerClientEntryHeader(const ola::acn::CID &client_cid)
        : m_client_cid(client_cid) {
    }
    ~BrokerClientEntryHeader() {}

    const ola::acn::CID ClientCid() const { return m_client_cid; }

    bool operator==(const BrokerClientEntryHeader &other) const {
      return m_client_cid == other.m_client_cid;
    }

    PACK(
    struct broker_client_entry_pdu_header_s {
      uint8_t client_cid[CID::CID_LENGTH];
    });
    typedef struct broker_client_entry_pdu_header_s broker_client_entry_pdu_header;

 private:
    ola::acn::CID m_client_cid;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_BROKERCLIENTENTRYHEADER_H_
