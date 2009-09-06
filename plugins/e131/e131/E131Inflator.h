/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E131Inflator.h
 * Interface for the E131Inflator class.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef OLA_E131_E131INFLATOR_H
#define OLA_E131_E131INFLATOR_H

#include "BaseInflator.h"
#include "E131Header.h"

namespace ola {
namespace e131 {

class E131Inflator: public BaseInflator {
  friend class E131InflatorTest;

  public:
    E131Inflator(): BaseInflator(),
                    m_last_header_valid(false) {
    }
    ~E131Inflator() {}

    unsigned int static const E131_VECTOR = 3;
    uint32_t Id() const { return E131_VECTOR; }

  protected:
    bool DecodeHeader(HeaderSet &headers, const uint8_t *data,
                      unsigned int len, unsigned int &bytes_used);

    virtual void ResetHeaderField();
  private :
    E131Header m_last_header;
    bool m_last_header_valid;
};

} // e131
} // ola

#endif
