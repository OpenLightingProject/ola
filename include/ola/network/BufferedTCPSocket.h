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
 * BufferedTCPSocket.h
 * A buffered TCP Socket
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_BUFFEREDTCPSOCKET_H_
#define INCLUDE_OLA_NETWORK_BUFFEREDTCPSOCKET_H_

#include <ola/network/Socket.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/io/BufferedWriteDescriptor.h>

namespace ola {
namespace network {

/*
 * A BufferedTCPSocket.
 * This is a copy and paste from the BufferedWriteDescriptor class, but I
 * couldn't find another way to do it :(.
 */
class BufferedTCPSocket: public TCPSocket, public ola::io::DescriptorStream {
  public:
    BufferedTCPSocket(int fd, ola::io::SelectServerInterface *ss = NULL)
      : TCPSocket(fd),
        DescriptorStream(ss) {
    }

    bool Close() {
      Disassociate();
      return TCPSocket::Close();
    }

    // We override Send() and buffer the data.
    ssize_t Send(const uint8_t *buffer, unsigned int size) {
      m_output_buffer.Write(buffer, size);
      if (!m_associated && size)
        Associate();
      return size;
    }

    void PerformWrite() {
      int iocnt;
      const struct iovec *iov = m_output_buffer.AsIOVec(&iocnt);
      ssize_t bytes_written = this->SendV(iov, iocnt);
      m_output_buffer.FreeIOVec(iov);
      m_output_buffer.Pop(bytes_written);

      if (m_output_buffer.Empty())
        Disassociate();
    }

  protected:
    void Associate() {
      m_ss->AddWriteDescriptor(this);
      m_associated = true;
    }

    void Disassociate() {
      if (m_associated && m_ss) {
        m_ss->RemoveWriteDescriptor(this);
        m_associated = false;
      }
    }
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_BUFFEREDTCPSOCKET_H_
