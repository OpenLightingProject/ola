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
 * E133StreamSender.h
 * This sends PDUs over a stream connection using E1.33.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef TOOLS_E133_E133STREAMSENDER_H_
#define TOOLS_E133_E133STREAMSENDER_H_

#include <map>
#include <string>

namespace ola {
namespace plugin {
namespace e131 {
  class RootSender;
  class OutgoingStreamTransport;
  class PDU;
};
};
};

/**
 * The E133StreamSender sends PDUs encapsulated with E1.33 headers over a
 * stream.
 *
 * Sequence numbers persist across transports.
 */
class E133StreamSender {
  public:
    E133StreamSender(ola::plugin::e131::RootSender *root_sender,
                     const std::string source_name);
    virtual ~E133StreamSender();

    void SetTransport(ola::plugin::e131::OutgoingStreamTransport *transport);

    bool Send(unsigned int vector,
              uint16_t endpoint,
              const class ola::plugin::e131::PDU &pdu);

  protected:
    unsigned int m_next_sequence_number;
    ola::plugin::e131::OutgoingStreamTransport *m_transport;

    bool SendPDU(unsigned int vector,
                 uint16_t endpoint,
                 unsigned int sequence_number,
                 const class ola::plugin::e131::PDU *pdu);
    virtual void NewTransport() {}

  private:
    const std::string m_source_name;
    ola::plugin::e131::RootSender *m_root_sender;

    E133StreamSender(const E133StreamSender&);
    E133StreamSender& operator=(const E133StreamSender&);
};



/**
 * A ReliableE133StreamSender adds an additional message which sends 'reliable'
 * messages over a stream. The Sender will buffer the message until we get an
 * explicit ACK (indicated with a call to Acknowledge() ). Messages will be
 * resent when a new transport becomes available.
 *
 * Sequence numbers persist across transports.
 */
class ReliableE133StreamSender: public E133StreamSender {
  public:
    ReliableE133StreamSender(ola::plugin::e131::RootSender *root_sender,
                             const std::string source_name,
                             unsigned int max_queue_size = MAX_QUEUE_SIZE);
    ~ReliableE133StreamSender();

    void Acknowledge(unsigned int sequence);

    bool SendReliability(unsigned int vector,
                         uint16_t endpoint,
                         const class ola::plugin::e131::PDU *pdu);

    unsigned int BufferSize() const;
    unsigned int FreeSize() const;

  protected:
    void NewTransport();

  private:
    typedef struct {
      unsigned int vector;
      uint16_t endpoint;
      const class ola::plugin::e131::PDU *pdu;
    } PendingMessage;

    typedef std::map<unsigned int, PendingMessage*> PendingMessageMap;

    unsigned int m_max_buffer_size;
    PendingMessageMap m_unacked_messages;

    ReliableE133StreamSender(const ReliableE133StreamSender&);
    ReliableE133StreamSender& operator=(const ReliableE133StreamSender&);

    void CleanupPendingMessage(PendingMessage *message);

    static const unsigned int MAX_QUEUE_SIZE = 10;
};
#endif  // TOOLS_E133_E133STREAMSENDER_H_
