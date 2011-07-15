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
 * MessageSerializer.h
 * Serialize a message.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_MESSAGESERIALIER_H_
#define INCLUDE_OLA_RDM_MESSAGESERIALIER_H_

#include <ola/messaging/MessageVisitor.h>

namespace ola {
namespace rdm {


/**
 * This visitor serializes the message.
 */
class MessageSerializer: public ola::messaging::MessageVisitor {
  public:
    MessageSerializer(unsigned int initial_size = INITIAL_BUFFER_SIZE);
    ~MessageSerializer();

    const uint8_t *SerializeMessage(const ola::messaging::Message *message,
                                    unsigned int *length);

    void Visit(const ola::messaging::BoolMessageField*);
    void Visit(const ola::messaging::StringMessageField*);
    void Visit(const ola::messaging::BasicMessageField<uint8_t>*);
    void Visit(const ola::messaging::BasicMessageField<uint16_t>*);
    void Visit(const ola::messaging::BasicMessageField<uint32_t>*);
    void Visit(const ola::messaging::BasicMessageField<int8_t>*);
    void Visit(const ola::messaging::BasicMessageField<int16_t>*);
    void Visit(const ola::messaging::BasicMessageField<int32_t>*);
    void Visit(const ola::messaging::GroupMessageField*);
    void PostVisit(const ola::messaging::GroupMessageField*);

  private:
    uint8_t *m_data;
    unsigned int m_offset, m_buffer_size, m_initial_buffer_size;

    static const unsigned int INITIAL_BUFFER_SIZE = 256;

    void CheckForFreeSpace(unsigned int required_size);

    template <typename int_type>
    void IntVisit(const ola::messaging::BasicMessageField<int_type> *);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_MESSAGESERIALIER_H_
