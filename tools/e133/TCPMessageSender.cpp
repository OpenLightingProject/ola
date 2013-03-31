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
 * E133StreamSender.cpp
 * The E133StreamSender
 * Copyright (C) 2011 Simon Newton
 */

#include <memory>

#include "ola/Logging.h"
#include "ola/io/IOStack.h"
#include "ola/rdm/RDMCommandSerializer.h"
#include "ola/stl/STLUtils.h"
#include "plugins/e131/e131/RDMPDU.h"

#include "tools/e133/TCPMessageSender.h"

using ola::rdm::RDMResponse;
using ola::io::IOStack;
using std::auto_ptr;

// The max number of un-ack'ed messages we'll allow.
const unsigned int TCPMessageSender::MAX_QUEUE_SIZE = 10;

// Track the un-ack'ed messages.
class OutstandingMessage {
  public:
    OutstandingMessage(uint16_t endpoint, const RDMResponse *rdm_response)
      : m_endpoint(endpoint),
        m_message_sent(false),
        m_rdm_response(rdm_response) {
    }

    bool was_sent() const { return m_message_sent; }
    void set_was_sent(bool was_sent) { m_message_sent = was_sent; }

    const RDMResponse* rdm_response() const { return m_rdm_response.get(); }
    uint16_t endpoint() const { return m_endpoint; }

  private:
    uint16_t m_endpoint;
    bool m_message_sent;
    auto_ptr<const RDMResponse> m_rdm_response;

    OutstandingMessage(const OutstandingMessage&);
    OutstandingMessage& operator=(const OutstandingMessage&);
};


//------------------------------------------------------------------------

/*
 * Create a new TCPMessageSender.
 * @param packet_builder the PacketBuilder to use, ownership of the pointer is
 *   not taken.
 * @param max_queue_size the max messages to store.
 */
TCPMessageSender::TCPMessageSender(PacketBuilder *packet_builder,
                                   unsigned int max_queue_size)
    : m_next_sequence_number(0),
      m_max_queue_size(max_queue_size),
      m_unsent_messages(false),
      m_packet_builder(packet_builder),
      m_message_queue(NULL) {
}


/**
 * Clean up
 */
TCPMessageSender::~TCPMessageSender() {
  if (!m_unacked_messages.empty())
    OLA_WARN << m_unacked_messages.size()
             << " RDM commands remain un-ack'ed and will not be delivered";
  ola::STLDeleteValues(&m_unacked_messages);
}


/**
 * Set the MessageQueue to use for sending RDM messages.
 */
void TCPMessageSender::SetMessageQueue(MessageQueue *message_queue) {
  m_message_queue = message_queue;
  if (m_message_queue) {
    OLA_INFO << "New connection, sending any un-acked messages";
    bool sent_all = true;
    PendingMessageMap::iterator iter = m_unacked_messages.begin();
    for (; iter != m_unacked_messages.end(); iter++) {
      OutstandingMessage *message = iter->second;
      bool was_sent = SendRDMCommand(iter->first, message->endpoint(),
                                     message->rdm_response());
      sent_all &= was_sent;
      message->set_was_sent(was_sent);
    }
    m_unsent_messages = !sent_all;
  }
}

/**
 * Called when something else acknowledges receipt of one of our messages
 */
void TCPMessageSender::Acknowledge(unsigned int sequence) {
  ola::STLRemoveAndDelete(&m_unacked_messages, sequence);
  if (m_unsent_messages && !m_message_queue->LimitReached()) {
    bool sent_all = true;
    PendingMessageMap::iterator iter = m_unacked_messages.begin();
    for (; iter != m_unacked_messages.end(); iter++) {
      OutstandingMessage *message = iter->second;
      if (message->was_sent())
        continue;
      bool was_sent = SendRDMCommand(iter->first, message->endpoint(),
                                     message->rdm_response());
      sent_all &= was_sent;
      message->set_was_sent(was_sent);
    }
    m_unsent_messages = !sent_all;
  }
}


/**
 * Send a RDM message reliably.
 * @param endpoint the endpoint to address this PDU to.
 * @param pdu the PDU itself, ownership is transferred if this returns true.
 */
bool TCPMessageSender::AddMessage(uint16_t endpoint,
                                  const ola::rdm::RDMResponse *rdm_response) {
  if (m_unacked_messages.size() == m_max_queue_size) {
    OLA_WARN << "MessageQueue limit reached, no further messages will be held";
    return false;
  }

  unsigned int our_sequence_number = m_next_sequence_number++;
  if (ola::STLContains(m_unacked_messages, our_sequence_number)) {
    // TODO(simon): think about what we want to do here
    OLA_WARN << "Sequence number collision!";
    return false;
  }

  OutstandingMessage *message = new OutstandingMessage(endpoint, rdm_response);
  ola::STLInsertIfNotPresent(&m_unacked_messages, our_sequence_number, message);

  if (m_message_queue) {
    message->set_was_sent(
      SendRDMCommand(our_sequence_number, endpoint, rdm_response));
  }
  return true;
}


/**
 * Return the number of un-acked messages waiting in the buffer
 */
unsigned int TCPMessageSender::QueueSize() const {
  return m_unacked_messages.size();
}


/**
 * Send a RDMResponse.
 */
bool TCPMessageSender::SendRDMCommand(unsigned int sequence_number,
                                      uint16_t endpoint,
                                      const RDMResponse *rdm_response) {
  if (m_message_queue->LimitReached())
    return false;

  IOStack packet(m_packet_builder->pool());
  ola::rdm::RDMCommandSerializer::Write(*rdm_response, &packet);
  ola::plugin::e131::RDMPDU::PrependPDU(&packet);
  m_packet_builder->BuildTCPRootE133(
      &packet, ola::plugin::e131::VECTOR_FRAMING_RDMNET, sequence_number,
      endpoint);

  return m_message_queue->SendMessage(&packet);
}
