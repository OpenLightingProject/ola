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
 * RDMPDU.h
 * Interface for the RDMPDU class
 * Copyright (C) 2012 Simon Newton
 */

#ifndef PLUGINS_E131_E131_RDMPDU_H_
#define PLUGINS_E131_E131_RDMPDU_H_

#include <ola/io/IOStack.h>
#include <ola/rdm/RDMCommand.h>
#include <memory>

#include "plugins/e131/e131/PDU.h"
#include "plugins/e131/e131/RDMInflator.h"

namespace ola {
namespace plugin {
namespace e131 {

/**
 * An RDM PDU carries a RDMCommand.
 */
class RDMPDU: public PDU {
 public:
    /**
     * Ownership of the command is transferred here
     */
    explicit RDMPDU(const ola::rdm::RDMCommand *command):
      PDU(RDMInflator::VECTOR_RDMNET_DATA, ONE_BYTE),
      m_command(command) {
    }
    ~RDMPDU() {
    }

    unsigned int HeaderSize() const { return 0; }
    unsigned int DataSize() const;
    bool PackHeader(uint8_t *data, unsigned int *length) const;
    bool PackData(uint8_t *data, unsigned int *length) const;

    void PackHeader(ola::io::OutputStream *stream) const {
      (void) stream;
    }

    void PackData(ola::io::OutputStream *stream) const;

    static void PrependPDU(ola::io::IOStack *stack);

 private:
    std::auto_ptr<const ola::rdm::RDMCommand> m_command;
};
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_RDMPDU_H_
