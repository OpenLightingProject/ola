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
 * E133StreamSender.cpp
 * The E133StreamSender
 * Copyright (C) 2011 Simon Newton
 */

#include <string>

#include "plugins/e131/e131/E131Includes.h"  //  NOLINT, this has to be first
#include "ola/Logging.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/E133PDU.h"
#include "plugins/e131/e131/RDMInflator.h"
#include "plugins/e131/e131/TCPTransport.h"
#include "plugins/e131/e131/RootSender.h"

#include "tools/e133/E133StreamSender.h"


using std::string;


/*
 * Create a new E133StreamSender
 * @param root_sender the root layer to use
 */
E133StreamSender::E133StreamSender(
    ola::plugin::e131::RootSender *root_sender,
    const string source_name)
    : m_next_sequence_number(0),
      m_transport(NULL),
      m_source_name(source_name),
      m_root_sender(root_sender) {
  if (!m_root_sender)
    OLA_FATAL << "root_sender is null, this won't work";
}


/**
 * Clean up
 */
E133StreamSender::~E133StreamSender() {}


/**
 * Set the transport to be used.
 * @param transport the transport to use, or NULL if we don't have one
 */
void E133StreamSender::SetTransport(
    ola::plugin::e131::OutgoingStreamTransport *transport) {
  m_transport = transport;
  NewTransport();
}


/*
 * Send a PDU unreliably.
 * @param vector the vector to use in the E1.33 header
 * @param endpoint the endpoint to address this PDU to
 * @param pdu the PDU itself.
 * @return true if the PDU was sent, false otherwise
 */
bool E133StreamSender::Send(unsigned int vector,
                            uint16_t endpoint,
                            const ola::plugin::e131::PDU &pdu) {
  if (!m_transport)
    return false;
  return SendPDU(vector, endpoint, m_next_sequence_number++, &pdu);
}


/**
 * Send a PDU using the transport.
 */
bool E133StreamSender::SendPDU(unsigned int vector,
                               uint16_t endpoint,
                               unsigned int sequence_number,
                               const class ola::plugin::e131::PDU *pdu) {
  ola::plugin::e131::E133Header header(m_source_name,
                                       sequence_number,
                                       endpoint,
                                       false);
  ola::plugin::e131::E133PDU e133_pdu(vector, header, pdu);

  return m_root_sender->SendPDU(
      ola::plugin::e131::E133Inflator::E133_VECTOR,
      e133_pdu,
      m_transport);
}


/*
 * Create a new ReliableE133StreamSender
 * @param root_sender the root layer to use
 */
ReliableE133StreamSender::ReliableE133StreamSender(
    ola::plugin::e131::RootSender *root_sender,
    const string source_name,
    unsigned int max_queue_size)
    : E133StreamSender(root_sender, source_name),
      m_max_buffer_size(max_queue_size) {
}


/**
 * Clean up
 */
ReliableE133StreamSender::~ReliableE133StreamSender() {
  if (!m_unacked_messages.empty())
    OLA_WARN << m_unacked_messages.size() <<
      " PDUs remaining in buffer and will not be delivered";

  PendingMessageMap::iterator iter = m_unacked_messages.begin();
  for (; iter != m_unacked_messages.end(); iter++)
    CleanupPendingMessage(iter->second);
  m_unacked_messages.clear();
}


/**
 * Called when something else acknowledges receipt of one of our messages
 */
void ReliableE133StreamSender::Acknowledge(unsigned int sequence) {
  PendingMessageMap::iterator iter = m_unacked_messages.find(sequence);
  if (iter != m_unacked_messages.end()) {
    CleanupPendingMessage(iter->second);
    m_unacked_messages.erase(iter);
  }
}

/**
 * Send a PDU reliably.
 * @param vector the vector to use in the E1.33 header
 * @param endpoint the endpoint to address this PDU to
 * @param pdu the PDU itself, ownership is transferred if this returns true.
 */
bool ReliableE133StreamSender::SendReliably(
    unsigned int vector,
    uint16_t endpoint,
    const ola::plugin::e131::PDU *pdu) {

  unsigned int our_sequence_number = m_next_sequence_number++;
  PendingMessageMap::iterator iter =
    m_unacked_messages.find(our_sequence_number);
  if (iter != m_unacked_messages.end()) {
    // TODO(simon): think about what we want to do here
    OLA_WARN << "Sequence number collision!";
    return false;
  }

  PendingMessage *message = new PendingMessage;
  message->vector = vector;
  message->endpoint = endpoint;
  message->pdu = pdu;

  m_unacked_messages[our_sequence_number] = message;

  if (m_transport)
    SendPDU(vector, endpoint, our_sequence_number, pdu);
  return true;
}


/**
 * Return the number of un-acked PDUs waiting in the buffer
 */
unsigned int ReliableE133StreamSender::BufferSize() const {
  return m_unacked_messages.size();
}


/**
 * Return the number of PDUs that can be added before the send buffer is full
 */
unsigned int ReliableE133StreamSender::FreeSize() const {
  return m_max_buffer_size - m_unacked_messages.size();
}


/**
 * Called when we get a new transport, send any pending messages
 */
void ReliableE133StreamSender::NewTransport() {
  if (m_transport) {
    // try to send all the pdus in the queue
    OLA_WARN << "New transport, sending any un-acked messages";
    PendingMessageMap::iterator iter = m_unacked_messages.begin();
    for (; iter != m_unacked_messages.end(); iter++)
      SendPDU(iter->second->vector,
              iter->second->endpoint,
              iter->first,
              iter->second->pdu);
  }
}


/**
 * Delete a pending message. This deletes the entire structure, the point is
 * invalid once this returns.
 */
void ReliableE133StreamSender::CleanupPendingMessage(PendingMessage *message) {
  delete message->pdu;
  delete message;
}
