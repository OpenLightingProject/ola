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
 * The RDMPDU class
 * Copyright (C) 2012 Simon Newton
 */

#ifndef LIBS_ACN_RDMPDU_H_
#define LIBS_ACN_RDMPDU_H_

#include <string>
#include <ola/io/ByteString.h>
#include <ola/io/IOStack.h>
#include <ola/rdm/RDMPacket.h>

#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

class RDMPDU : public PDU {
 public:
  explicit RDMPDU(const ola::io::ByteString &command):
    PDU(ola::rdm::START_CODE, ONE_BYTE, true),
    m_command(command) {}

  unsigned int HeaderSize() const { return 0; }
  bool PackHeader(OLA_UNUSED uint8_t *data,
                  unsigned int *length) const {
    *length = 0;
    return true;
  }
  void PackHeader(OLA_UNUSED ola::io::OutputStream *stream) const {}

  unsigned int DataSize() const;
  bool PackData(uint8_t *data, unsigned int *length) const;
  void PackData(ola::io::OutputStream *stream) const;

  static void PrependPDU(ola::io::IOStack *stack);

 private:
  const ola::io::ByteString m_command;
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_RDMPDU_H_
