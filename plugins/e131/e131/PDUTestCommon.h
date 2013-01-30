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
 * PDUTestCommon.cpp
 * Provides a simple PDU class for testing
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef PLUGINS_E131_E131_PDUTESTCOMMON_H_
#define PLUGINS_E131_E131_PDUTESTCOMMON_H_

#include "ola/Callback.h"
#include "ola/io/OutputStream.h"
#include "plugins/e131/e131/BaseInflator.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/PDU.h"

namespace ola {
namespace plugin {
namespace e131 {


/*
 * This isn't a PDU at all, it just packs a uint32 for testing.
 */
class FakePDU: public PDU {
  public:
    explicit FakePDU(unsigned int value): PDU(0), m_value(value) {}
    unsigned int Size() const { return sizeof(m_value); }
    unsigned int HeaderSize() const { return 0; }
    unsigned int DataSize() const { return 0; }

    bool Pack(uint8_t *data, unsigned int &length) const {
      if (length < sizeof(m_value))
        return false;
      memcpy(data, &m_value, sizeof(m_value));
      length = sizeof(m_value);
      return true;
    }

    bool PackHeader(uint8_t*, unsigned int&) const {
      return true;
    }

    bool PackData(uint8_t*, unsigned int&) const {
      return true;
    }

    void Write(ola::io::OutputStream *stream) const {
      *stream << HostToNetwork(m_value);
    }

    void PackHeader(ola::io::OutputStream*) const {}
    void PackData(ola::io::OutputStream*) const {}

  private:
    unsigned int m_value;
};


/*
 * A Mock PDU class that can be used for testing.
 * Mock PDUs have a 4 byte header, and a 4 byte data payload which the inflator
 * will check is 2x the header value.
 */
class MockPDU: public PDU {
  public:
    MockPDU(unsigned int header, unsigned int value):
      PDU(TEST_DATA_VECTOR),
      m_header(header),
      m_value(value) {}
    unsigned int HeaderSize() const { return sizeof(m_header); }
    unsigned int DataSize() const { return sizeof(m_value); }

    bool PackHeader(uint8_t *data, unsigned int &length) const {
      if (length < HeaderSize()) {
        length = 0;
        return false;
      }
      memcpy(data, &m_header, sizeof(m_header));
      length = HeaderSize();
      return true;
    }

    void PackHeader(ola::io::OutputStream *stream) const {
      stream->Write(reinterpret_cast<const uint8_t*>(&m_header),
                    sizeof(m_header));
    }

    bool PackData(uint8_t *data, unsigned int &length) const {
      if (length < DataSize()) {
        length = 0;
        return false;
      }
      memcpy(data, &m_value, sizeof(m_value));
      length = DataSize();
      return true;
    }

    void PackData(ola::io::OutputStream *stream) const {
      stream->Write(reinterpret_cast<const uint8_t*>(&m_value),
                    sizeof(m_value));
    }

    // This is used to id 'Mock' PDUs in the higher level protocol
    static const unsigned int TEST_VECTOR = 42;
    // This is is the vector used by MockPDUs
    static const unsigned int TEST_DATA_VECTOR = 43;

  private:
    unsigned int m_header;
    unsigned int m_value;
};


/*
 * The inflator the works with MockPDUs. We check that the data = 2 * header
 */
class MockInflator: public BaseInflator {
  public:
    MockInflator(const CID &cid, Callback0<void> *on_recv = NULL):
      BaseInflator(),
      m_cid(cid),
      m_on_recv(on_recv) {}
    uint32_t Id() const { return MockPDU::TEST_VECTOR; }

  protected:
    void ResetHeaderField() {}
    bool DecodeHeader(HeaderSet &,
                      const uint8_t *data,
                      unsigned int,
                      unsigned int &bytes_used) {
      if (data) {
        bytes_used = 4;
        memcpy(&m_last_header, data, sizeof(m_last_header));
      }
      return true;
    }

    bool HandlePDUData(uint32_t vector, HeaderSet &headers,
                       const uint8_t *data, unsigned int pdu_length) {
      CPPUNIT_ASSERT_EQUAL((uint32_t) MockPDU::TEST_DATA_VECTOR, vector);
      CPPUNIT_ASSERT_EQUAL((unsigned int) 4, pdu_length);
      unsigned int *value = (unsigned int*) data;
      CPPUNIT_ASSERT_EQUAL(m_last_header * 2, *value);

      if (!m_cid.IsNil()) {
        // check the CID as well
        RootHeader root_header = headers.GetRootHeader();
        CPPUNIT_ASSERT(m_cid == root_header.GetCid());
      }

      if (m_on_recv)
        m_on_recv->Run();
      return true;
    }

  private:
    CID m_cid;
    Callback0<void> *m_on_recv;
    unsigned int m_last_header;
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_PDUTESTCOMMON_H_
