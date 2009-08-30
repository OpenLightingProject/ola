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
 * PDU.h
 * Interface for the PDU and PDUBlock classes
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_E131_PDU_H
#define OLA_E131_PDU_H

#include <stdint.h>
#include <vector>

namespace ola {
namespace e131 {


/*
 * The Base PDU class
 */
class PDU {
  public:
    PDU() {}
    virtual ~PDU() {}

    // Returns the size of this PDU
    virtual unsigned int Size() const = 0;
    /*
     * Pack the PDU into the memory pointed to by data
     * @return true on success, false on failure
     */
    virtual bool Pack(uint8_t *data, unsigned int &length) const = 0;
    static const int TWOB_LENGTH_LIMIT = 4095;
};


/*
 * Represents a block of pdus
 */
template <class C>
class PDUBlock {
  public:
    PDUBlock() {}
    ~PDUBlock() {}

    // Add a PDU to this block
    void AddPDU(C *msg) { m_pdus.push_back(msg); }
    // Remove all PDUs from the block
    void Clear() { m_pdus.clear(); }
    // The number of bytes this block would consume
    unsigned int Size() const;
    /*
     * Pack this PDUBlock into memory pointed to by data
     * @return true on success, false on failure
     */
    bool Pack(uint8_t *data, unsigned int &length) const;

  private:
    std::vector<C*> m_pdus;
};


/*
 * return the size of this pdu block
 * @return size of the pdu block
 */
template <class C>
unsigned int PDUBlock<C>::Size() const {
  // We trade performance for consistency here, if this becomes a problem we
  // can calculate the size when the PDU is added
  unsigned int size = 0;
  typename std::vector<C*>::const_iterator iter;
  for (iter = m_pdus.begin(); iter != m_pdus.end(); ++iter) {
    size += (*iter)->Size();
  }
  return size;
}


/*
 * Pack this block of PDUs into a buffer
 * @param data a pointer to the buffer
 * @param length size of the buffer, updated with the number of bytes used
 * @return true on success, false on failure
 */
template <class C>
bool PDUBlock<C>::Pack(uint8_t *data, unsigned int &length) const {
  bool status = true;
  unsigned int i = 0;
  typename std::vector<C*>::const_iterator iter;
  for (iter = m_pdus.begin(); iter != m_pdus.end(); ++iter) {
    // TODO: optimize repeated headers & vectors here
    unsigned int remaining = i < length ? length - i : 0;
    status &= (*iter)->Pack(data + i, remaining);
    i+= remaining;
  }
  length = i;
  return status;
}

} // e131
} // ola

#endif
