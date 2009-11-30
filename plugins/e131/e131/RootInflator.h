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
 * RootInflator.h
 * Interface for the RootInflator class.
 * Copyright (C) 2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_ROOTINFLATOR_H_
#define PLUGINS_E131_E131_ROOTINFLATOR_H_

#include "plugins/e131/e131/BaseInflator.h"

namespace ola {
namespace plugin {
namespace e131 {

class RootInflator: public BaseInflator {
  public:
    RootInflator(): BaseInflator() {}
    ~RootInflator() {}
    uint32_t Id() const { return 0; }  // no effect for the root inflator

  protected:
    // Decode a header block and adds any PduHeaders to the HeaderSet object
    bool DecodeHeader(HeaderSet &headers, const uint8_t *data,
                      unsigned int len, unsigned int &bytes_used);

    virtual void ResetHeaderField();
  private :
    RootHeader m_last_hdr;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_ROOTINFLATOR_H_
