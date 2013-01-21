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
 * E133Inflator.h
 * Interface for the E133Inflator class.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E133INFLATOR_H_
#define PLUGINS_E131_E131_E133INFLATOR_H_

#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/E133Header.h"

namespace ola {
namespace plugin {
namespace e131 {

class E133Inflator: public BaseInflator {
  friend class E133InflatorTest;

  public:
    static const unsigned int E133_VECTOR = 5;

    E133Inflator()
        : BaseInflator(),
          m_last_header_valid(false) {
    }
    ~E133Inflator() {}

    uint32_t Id() const { return E133_VECTOR; }

  protected:
    bool DecodeHeader(HeaderSet &headers,
                      const uint8_t *data,
                      unsigned int len,
                      unsigned int &bytes_used);

    void ResetHeaderField() {
      m_last_header_valid = false;
    }
  private:
    E133Header m_last_header;
    bool m_last_header_valid;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E133INFLATOR_H_
