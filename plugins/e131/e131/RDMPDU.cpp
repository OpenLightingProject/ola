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
 * RDMPDU.cpp
 * The RDMPDU
 * Copyright (C) 2012 Simon Newton
 */


#include <string.h>
#include <ola/Logging.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMCommandSerializer.h>
#include "plugins/e131/e131/RDMPDU.h"

namespace ola {
namespace plugin {
namespace e131 {

using ola::io::OutputStream;
using ola::network::HostToNetwork;
using ola::rdm::RDMCommandSerializer;

/*
 * Size of the data portion
 */
unsigned int RDMPDU::DataSize() const {
  if (m_command.get())
    return RDMCommandSerializer::RequiredSize(*m_command);
  return 0;
}


/*
 * RDM PDUs don't contain a header.
 */
bool RDMPDU::PackHeader(uint8_t *, unsigned int *length) const {
  *length = 0;
  return true;
}


/*
 * Pack the data portion.
 */
bool RDMPDU::PackData(uint8_t *data, unsigned int *length) const {
  if (!m_command.get()) {
    *length = 0;
    return true;
  }

  return RDMCommandSerializer::Pack(*m_command, data, length);
}


/*
 * Pack the data into a buffer
 */
void RDMPDU::PackData(OutputStream *stream) const {
  if (!m_command.get())
    return;
  m_command->Write(stream);
}


void RDMPDU::PrependPDU(ola::io::IOStack *stack) {
  uint8_t vector = HostToNetwork(ola::rdm::RDMCommand::START_CODE);
  stack->Write(reinterpret_cast<uint8_t*>(&vector), sizeof(vector));
  PrependFlagsAndLength(stack);
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
