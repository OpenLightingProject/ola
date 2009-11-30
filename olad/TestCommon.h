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
 * UniverseTest.cpp
 * Test fixture for the Universe and UniverseStore classes
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLAD_TESTCOMMON_H_
#define OLAD_TESTCOMMON_H_
#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/DmxBuffer.h"
#include "olad/Port.h"

using ola::AbstractDevice;
using ola::DmxBuffer;
using ola::InputPort;
using ola::OutputPort;
using std::string;


class TestMockInputPort: public InputPort {
  public:
    TestMockInputPort(AbstractDevice *parent, unsigned int port_id):
      InputPort(parent, port_id) {}
    ~TestMockInputPort() {}

    string Description() const { return ""; }
    bool WriteDMX(const DmxBuffer &buffer) { m_buffer = buffer; }
    const DmxBuffer &ReadDMX() const { return m_buffer; }

  private:
    DmxBuffer m_buffer;
};


class TestMockOutputPort: public OutputPort {
  public:
    TestMockOutputPort(AbstractDevice *parent, unsigned int port_id):
      OutputPort(parent, port_id) {}
    ~TestMockOutputPort() {}

    string Description() const { return ""; }
    bool WriteDMX(const DmxBuffer &buffer) { m_buffer = buffer; }
    const DmxBuffer &ReadDMX() const { return m_buffer; }

  private:
    DmxBuffer m_buffer;
};
#endif  // OLAD_TESTCOMMON_H_
