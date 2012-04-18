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
 * RDMPDU.h
 * Interface for the RDMPDU class
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_E131_E131_RDMPDU_H_
#define PLUGINS_E131_E131_RDMPDU_H_

#include <ola/rdm/RDMCommand.h>
#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/RDMInflator.h"

namespace ola {
namespace plugin {
namespace e131 {

class DMPPDU;

class RDMPDU: public PDU {
  public:
    explicit RDMPDU(const ola::rdm::RDMCommand *command):
      PDU(RDMInflator::RDM_DATA_VECTOR),
      m_command(command) {
    }
    ~RDMPDU() {}

    unsigned int HeaderSize() const { return 0; }
    unsigned int DataSize() const;
    bool PackHeader(uint8_t *data, unsigned int &length) const;
    bool PackData(uint8_t *data, unsigned int &length) const;

  private:
    const ola::rdm::RDMCommand *m_command;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_RDMPDU_H_
