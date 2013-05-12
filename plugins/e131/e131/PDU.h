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
 * PDU.h
 * Interface for the PDU and PDUBlock classes
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_PDU_H_
#define PLUGINS_E131_E131_PDU_H_

#include <stdint.h>
#include <ola/io/OutputStream.h>
#include <ola/io/OutputBuffer.h>
#include <vector>

namespace ola {
namespace plugin {
namespace e131 {


using ola::io::OutputStream;


/*
 * The Base PDU class
 * TODO(simon): make this into a template based on vector size.
 */
class PDU {
  public:
    typedef enum {
      ONE_BYTE = 1,
      TWO_BYTES = 2,
      FOUR_BYTES = 4,
    } vector_size;

    PDU(unsigned int vector, vector_size size = FOUR_BYTES):
      m_vector(vector),
      m_vector_size(size) {}
    virtual ~PDU() {}

    // Returns the size of this PDU
    virtual unsigned int Size() const;
    virtual unsigned int VectorSize() const { return m_vector_size; }
    virtual unsigned int HeaderSize() const = 0;
    virtual unsigned int DataSize() const = 0;

    // Set the vector
    void SetVector(unsigned int vector) { m_vector = vector; }

    /*
     * Pack the PDU into the memory pointed to by data
     * @return true on success, false on failure
     */
    virtual bool Pack(uint8_t *data, unsigned int *length) const;
    virtual bool PackHeader(uint8_t *data, unsigned int *length) const = 0;
    virtual bool PackData(uint8_t *data, unsigned int *length) const = 0;

    /**
     * Write the PDU to an OutputStream
     */
    virtual void Write(OutputStream *stream) const;
    virtual void PackHeader(OutputStream *stream) const = 0;
    virtual void PackData(OutputStream *stream) const = 0;

    static void PrependFlagsAndLength(
        ola::io::OutputBufferInterface *output,
        uint8_t flags = VFLAG_MASK | HFLAG_MASK | DFLAG_MASK);

    static void PrependFlagsAndLength(
        ola::io::OutputBufferInterface *output,
        unsigned int length,
        uint8_t flags);

    // This indicates a vector is present
    static const uint8_t VFLAG_MASK = 0x40;
    // This indicates a header field is present
    static const uint8_t HFLAG_MASK = 0x20;
    // This indicates a data field is present
    static const uint8_t DFLAG_MASK = 0x10;

  private:
    unsigned int m_vector;
    unsigned int m_vector_size;

    // The max PDU length that can be represented with the 2 byte format for
    // the length field.
    static const unsigned int TWOB_LENGTH_LIMIT = 0x0FFF;
};


/*
 * Represents a block of pdus
 */
template <class C>
class PDUBlock {
  public:
    PDUBlock(): m_size(0) {}
    ~PDUBlock() {}

    // Add a PDU to this block
    void AddPDU(const C *msg) {
      m_pdus.push_back(msg);
      m_size += msg->Size();
    }
    // Remove all PDUs from the block
    void Clear() {
      m_pdus.clear();
      m_size = 0;
    }
    // The number of bytes this block would consume, this ignores optimizations
    // like repeating headers/vectors.
    unsigned int Size() const { return m_size; }
    /*
     * Pack this PDUBlock into memory pointed to by data
     * @return true on success, false on failure
     */
    bool Pack(uint8_t *data, unsigned int *length) const;

    /**
     * Write this PDU block to an OutputStream
     */
    void Write(OutputStream *stream) const;

  private:
    std::vector<const C*> m_pdus;
    unsigned int m_size;
};


/*
 * Pack this block of PDUs into a buffer
 * @param data a pointer to the buffer
 * @param length size of the buffer, updated with the number of bytes used
 * @return true on success, false on failure
 */
template <class C>
bool PDUBlock<C>::Pack(uint8_t *data, unsigned int *length) const {
  bool status = true;
  unsigned int i = 0;
  typename std::vector<const C*>::const_iterator iter;
  for (iter = m_pdus.begin(); iter != m_pdus.end(); ++iter) {
    // TODO(simon): optimize repeated headers & vectors here
    unsigned int remaining = i < *length ? *length - i : 0;
    status &= (*iter)->Pack(data + i, &remaining);
    i+= remaining;
  }
  *length = i;
  return status;
}


/*
 * Write this block of PDUs to an OutputStream.
 * @param stream the OutputStream to write to
 * @return true on success, false on failure
 */
template <class C>
void PDUBlock<C>::Write(OutputStream *stream) const {
  typename std::vector<const C*>::const_iterator iter;
  for (iter = m_pdus.begin(); iter != m_pdus.end(); ++iter) {
    // TODO(simon): optimize repeated headers & vectors here
    (*iter)->Write(stream);
  }
}
}  // namespace e131
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_E131_E131_PDU_H_
