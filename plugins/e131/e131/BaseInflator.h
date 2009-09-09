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
 * BaseInflator.h
 * Provides the base class for inflating PDU blocks.
 * Copyright (C) 2007-2009 Simon Newton
 *
 * The BaseInflator takes care of most of the heavy lifting when inflating PDU
 * blocks. To create a specific Inflator, subclass BaseInflator and implement
 * the Id() and DecodeHeader() methods.
 */

#ifndef OLA_E131_BASEINFLATOR_H
#define OLA_E131_BASEINFLATOR_H

#include <stdint.h>
#include <map>

#include "HeaderSet.h"
#include "PDU.h"

namespace ola {
namespace e131 {

class BaseInflatorTest;


/*
 * An abstract PDU inflator
 */
class BaseInflator {
  friend class BaseInflatorTest;

  public:
    BaseInflator(PDU::vector_size v_size=PDU::FOUR_BYTES);
    virtual ~BaseInflator() {};

    /*
     * Add another inflator as a handler
     */
    bool AddInflator(class BaseInflator *inflator);
    /*
     * Return the inflator used for a particular protocol
     */
    class BaseInflator *GetInflator(uint32_t proto) const;

    /*
     * Return the id for this inflator
     */
    virtual uint32_t Id() const = 0;

    /*
     * Parse a block of PDU data
     */
    virtual int InflatePDUBlock(HeaderSet &headers, const uint8_t *data,
                                unsigned int len);

    // masks for the flag fields
    // This indicates a 20 bit length field (default is 12 bits)
    static const uint8_t LFLAG_MASK = 0x80;
    // This masks the first 4 bits of the length field
    static const uint8_t LENGTH_MASK = 0x0F;

  protected:
    uint32_t m_last_vector;
    bool m_vector_set;
    PDU::vector_size m_vector_size; // size of the vector field
    // map protos to inflators
    std::map<uint32_t, class BaseInflator*> m_proto_map;

    // Reset repeated pdu fields
    virtual void ResetPDUFields();
    virtual void ResetHeaderField() = 0;

    // determine the length of a pdu
    bool DecodeLength(const uint8_t *data, unsigned int data_length,
                      unsigned int &pdu_length,
                      unsigned int &bytes_used) const;

    // determine the vector of a pdu
    bool DecodeVector(uint8_t flags, const uint8_t *data, unsigned int length,
                      uint32_t &vector, unsigned int &bytes_used);

    // Decode a header block and adds any PduHeaders to the HeaderSet object
    virtual bool DecodeHeader(HeaderSet &headers, const uint8_t *data,
                              unsigned int len, unsigned int &bytes_used) = 0;

    // parse the body of a pdu
    bool InflatePDU(HeaderSet &headers, uint8_t flags, const uint8_t *data,
                    unsigned int pdu_len);

    // called after the header is parsed
    virtual bool PostHeader(uint32_t vector, HeaderSet &headers);

    // called in the absence of a parse to handle the pdu data
    virtual bool HandlePDUData(uint32_t vector, HeaderSet &headers,
                               const uint8_t *data, unsigned int pdu_len);
};

} // e131
} // ola
#endif
