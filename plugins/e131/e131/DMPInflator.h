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
 * DMPInflator.h
 * Interface for the DMPInflator class.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_DMPINFLATOR_H_
#define PLUGINS_E131_E131_DMPINFLATOR_H_

#include "ola/acn/ACNVectors.h"
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/DMPHeader.h"

namespace ola {
namespace plugin {
namespace e131 {

class DMPInflator: public BaseInflator {
  friend class DMPInflatorTest;

 public:
    DMPInflator(): BaseInflator(PDU::ONE_BYTE),
                   m_last_header_valid(false) {
    }
    virtual ~DMPInflator() {}

    uint32_t Id() const { return ola::acn::VECTOR_E131_DATA; }

 protected:
    bool DecodeHeader(HeaderSet *headers,
                      const uint8_t *data,
                      unsigned int len,
                      unsigned int *bytes_used);

    void ResetHeaderField();
 private:
    DMPHeader m_last_header;
    bool m_last_header_valid;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_DMPINFLATOR_H_
