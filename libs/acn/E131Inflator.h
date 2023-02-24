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
 * E131Inflator.h
 * Interface for the E131Inflator class.
 * Copyright (C) 2009 Simon Newton
 *
 * This contains two inflators a E131Inflator as per the standard and an
 * E131InflatorRev2 which implements the revision 2 draft specification.
 */

#ifndef LIBS_ACN_E131INFLATOR_H_
#define LIBS_ACN_E131INFLATOR_H_

#include "ola/acn/ACNVectors.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/E131Header.h"

namespace ola {
namespace acn {

class E131Inflator: public BaseInflator {
  friend class E131InflatorTest;

 public:
    E131Inflator(): BaseInflator(),
                    m_last_header_valid(false) {
    }
    ~E131Inflator() {}

    uint32_t Id() const { return ola::acn::VECTOR_ROOT_E131; }

 protected:
    bool DecodeHeader(HeaderSet *headers,
                      const uint8_t *data,
                      unsigned int len,
                      unsigned int *bytes_used);

    void ResetHeaderField() {
      m_last_header_valid = false;
    }
 private:
    E131Header m_last_header;
    bool m_last_header_valid;
};


/*
 * A Revision 2 version of the inflator.
 */
class E131InflatorRev2: public BaseInflator {
  friend class E131InflatorTest;

 public:
    E131InflatorRev2(): BaseInflator(),
                    m_last_header_valid(false) {
    }
    ~E131InflatorRev2() {}

    uint32_t Id() const { return ola::acn::VECTOR_ROOT_E131_REV2; }

 protected:
    bool DecodeHeader(HeaderSet *headers, const uint8_t *data,
                      unsigned int len, unsigned int *bytes_used);

    void ResetHeaderField() {
      m_last_header_valid = false;
    }
 private:
    E131Header m_last_header;
    bool m_last_header_valid;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_E131INFLATOR_H_
