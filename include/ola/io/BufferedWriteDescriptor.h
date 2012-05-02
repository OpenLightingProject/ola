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
 * BufferedOutputDescriptor.h
 * 
 * Copyright (C) 2012 Simon Newton
 *
 */

#ifndef INCLUDE_OLA_IO_BUFFEREDWRITEDESCRIPTOR_H_
#define INCLUDE_OLA_IO_BUFFEREDWRITEDESCRIPTOR_H_

#include <stdint.h>

#include <string>
#include <ola/Callback.h>
#include <ola/io/IOQueue.h>
#include <ola/io/SelectServerInterface.h>


namespace ola {
namespace io {


/**
 * A BufferedOutputDescriptor inherits from a ConnectedDescriptor (why?) and
 * buffers the data in an IOQueue. When the queue is non-empty the class will
 * associate itself with the given SelectServer and perform non-blocking, on
 * demand writes.
 *
 * @tparam Parent the parent class to inherit from. The parent must provide a
 * Send() method to perform the actual write.
 */
template <typename Parent>
class BufferedOutputDescriptor: public Parent, OutputStream {
  public:
    BufferedOutputDescriptor(SelectServerInterface *ss = NULL)
      : m_associated(false),
        m_ss(ss) {
    }

    bool Close() {
      Disassociate();
      return Parent::Close();
    }

    /**
     * Associate a select server with this descriptor.
     */
    void AssociateSelectServer(SelectServerInterface *ss) {
      Disassociate();
      m_ss = ss;

      // if we've still got data, associate with this ss
      if (!m_output_buffer.Empty())
        Associate();
    }

    // Methods from OutputStream
    bool Empty() const { return m_output_buffer.Empty(); }
    unsigned int Size() const { return m_output_buffer.Size(); }
    void Write(const uint8_t *data, unsigned int length) { Send(data, length);
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

  private:
    bool m_associated;
    IOQueue m_output_buffer;
    SelectServerInterface *m_ss;

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


typedef BufferedOutputDescriptor<LoopbackDescriptor>
  BufferedLoopbackDescriptor;
}  // io
}  // ola
#endif  // INCLUDE_OLA_IO_BUFFEREDWRITEDESCRIPTOR_H_
