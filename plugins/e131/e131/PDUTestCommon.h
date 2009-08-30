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
 * PDUTestCommon.cpp
 * Provides a simple PDU class for testing
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef OLA_E131_PDUTESTCOMMON_H
#define OLA_E131_PDUTESTCOMMON_H

#include "PDU.h"

namespace ola {
namespace e131 {


/*
 * This PDU just contains a int32.
 */
class MockPDU: public PDU {
  public:
    MockPDU(unsigned int value): m_value(value) {}
    unsigned int Size() const { return sizeof(m_value); }

    bool Pack(uint8_t *data, unsigned int &length) const {
      if (length < sizeof(m_value))
        return false;
      unsigned int *int_data = (unsigned int*) data;
      *int_data = m_value;
      length = sizeof(m_value);
      return true;
    }

  private:
    unsigned int m_value;
};


} // e131
} // ola

#endif
