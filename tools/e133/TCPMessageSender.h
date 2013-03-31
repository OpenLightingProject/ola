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
 * TCPMessageSender.h
 * Copyright (C) 2011 Simon Newton
 *
 * This handles sending RDMPDUs over a TCP connection. It tracks which messages
 * have been acknowledged, and will re-send un-acknowledged messages when a new
 * MessageQueue is available.
 */

#ifndef TOOLS_E133_TCPMESSAGESENDER_H_
#define TOOLS_E133_TCPMESSAGESENDER_H_

#include <map>
#include "ola/rdm/RDMCommand.h"
#include "tools/e133/PacketBuilder.h"
#include "tools/e133/MessageQueue.h"

class TCPMessageSender {
  public:
    TCPMessageSender(PacketBuilder *packet_builder,
                     unsigned int max_queue_size = MAX_QUEUE_SIZE);
    ~TCPMessageSender();

    void SetMessageQueue(MessageQueue *message_queue);

    void Acknowledge(unsigned int sequence);

    bool AddMessage(uint16_t endpoint,
                    const ola::rdm::RDMResponse *rdm_response);

    unsigned int QueueSize() const;

  private:
    typedef std::map<unsigned int, class OutstandingMessage*> PendingMessageMap;

    unsigned int m_next_sequence_number;
    unsigned int m_max_queue_size;
    // Indicates if we have messages that haven't been sent on the MessageQueue
    // yet.
    bool m_unsent_messages;
    PacketBuilder *m_packet_builder;
    MessageQueue *m_message_queue;
    PendingMessageMap m_unacked_messages;

    bool SendRDMCommand(unsigned int sequence_number, uint16_t endpoint,
                        const ola::rdm::RDMResponse *rdm_response);

    TCPMessageSender(const TCPMessageSender&);
    TCPMessageSender& operator=(const TCPMessageSender&);

    static const unsigned int MAX_QUEUE_SIZE;
};
#endif  // TOOLS_E133_TCPMESSAGESENDER_H_
