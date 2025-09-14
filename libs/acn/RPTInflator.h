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
 * RPTInflator.h
 * Interface for the RPTInflator class.
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_RPTINFLATOR_H_
#define LIBS_ACN_RPTINFLATOR_H_

#include "ola/acn/ACNVectors.h"
#include "libs/acn/BaseInflator.h"
#include "libs/acn/RPTHeader.h"

namespace ola {
namespace acn {

class RPTInflator: public BaseInflator {
  friend class RPTInflatorTest;

 public:
    RPTInflator()
        : BaseInflator(),
          m_last_header_valid(false) {
    }
    ~RPTInflator() {}

    uint32_t Id() const { return ola::acn::VECTOR_ROOT_RPT; }

 protected:
    bool DecodeHeader(HeaderSet *headers,
                      const uint8_t *data,
                      unsigned int len,
                      unsigned int *bytes_used);

    void ResetHeaderField() {
      m_last_header_valid = false;
    }
 private:
    RPTHeader m_last_header;
    bool m_last_header_valid;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_RPTINFLATOR_H_
