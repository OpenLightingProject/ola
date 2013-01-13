/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * BufferedTCPSocket.h
 * A buffered TCP Socket
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_NETWORK_BUFFEREDTCPSOCKET_H_
#define INCLUDE_OLA_NETWORK_BUFFEREDTCPSOCKET_H_

#include <ola/io/BufferedWriteDescriptor.h>
#include <ola/io/IOQueue.h>
#include <ola/io/SelectServerInterface.h>
#include <ola/network/TCPSocket.h>

namespace ola {
namespace network {

/*
 * A BufferedTCPSocket.
 * This is a copy and paste from the BufferedWriteDescriptor class, but I
 * couldn't find another way to do it :(.
 *
 * This is because the constructor needs to take an int so that it works with
 * the TCPSocketFactory.
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
      TCPSocket::Send(&m_output_buffer);
      if (m_output_buffer.Empty())
        Disassociate();
    }

  protected:
    ConnectedDescriptor *GetDescriptor() { return this; }

  private:
    // this is prviate, since using a IOQueue with a BufferedTCPSocket would be
    // incur a copy so we avoid it.
    ssize_t Send(ola::io::IOQueue *) {
      return -1;
    }
};
}  // network
}  // ola
#endif  // INCLUDE_OLA_NETWORK_BUFFEREDTCPSOCKET_H_
