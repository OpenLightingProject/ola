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
 * E133PDU.h
 * Interface for the E133PDU class
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E133PDU_H_
#define PLUGINS_E131_E131_E133PDU_H_

#include <ola/io/IOStack.h>
#include <string>

#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/E133Header.h"

namespace ola {
namespace plugin {
namespace e131 {

class RDMPDU;

class E133PDU: public PDU {
  public:
    E133PDU(unsigned int vector,
            const E133Header &header,
            const PDU *pdu):
      PDU(vector),
      m_header(header),
      m_pdu(pdu) {}
    ~E133PDU() {}

    unsigned int HeaderSize() const;
    unsigned int DataSize() const;
    bool PackHeader(uint8_t *data, unsigned int *length) const;
    bool PackData(uint8_t *data, unsigned int *length) const;

    void PackHeader(OutputStream *stream) const;
    void PackData(OutputStream *stream) const;

    static void PrependPDU(ola::io::IOStack *stack, uint32_t vector,
                           const string &source, uint32_t sequence_number,
                           uint16_t endpoint_id);

  private:
    E133Header m_header;
    const PDU *m_pdu;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_E133PDU_H_
