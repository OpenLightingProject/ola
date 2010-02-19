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
#include "olad/Device.h"
#include "olad/Plugin.h"
#include "olad/Port.h"
#include "olad/PortManager.h"

using ola::AbstractDevice;
using ola::DmxBuffer;
using ola::InputPort;
using ola::OutputPort;
using std::string;


/*
 * Mock out an Input Port
 */
class TestMockInputPort: public InputPort {
  public:
    TestMockInputPort(AbstractDevice *parent, unsigned int port_id):
      InputPort(parent, port_id) {}
    ~TestMockInputPort() {}

    string Description() const { return ""; }
    bool WriteDMX(const DmxBuffer &buffer) {
      m_buffer = buffer;
      return true;
    }
    const DmxBuffer &ReadDMX() const { return m_buffer; }

  private:
    DmxBuffer m_buffer;
};


/*
 * Same as above but this supports priorities
 */
class TestMockPriorityInputPort: public TestMockInputPort {
  public:
    TestMockPriorityInputPort(AbstractDevice *parent, unsigned int port_id):
        TestMockInputPort(parent, port_id),
        m_inherited_priority(ola::DmxSource::PRIORITY_DEFAULT) {
    }

    uint8_t InheritedPriority() const {
      return m_inherited_priority;
    }

    void SetInheritedPriority(uint8_t priority) {
      m_inherited_priority = priority;
    }

  protected:
    bool SupportsPriorities() const { return true; }

  private:
    uint8_t m_inherited_priority;
};


/*
 * Mock out an OutputPort
 */
class TestMockOutputPort: public OutputPort {
  public:
    TestMockOutputPort(AbstractDevice *parent, unsigned int port_id):
      OutputPort(parent, port_id) {}
    ~TestMockOutputPort() {}

    string Description() const { return ""; }
    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      m_buffer = buffer;
      (void) priority;
      return true;
    }
    const DmxBuffer &ReadDMX() const { return m_buffer; }

  private:
    DmxBuffer m_buffer;
};


/*
 * Same as above but this supports priorities
 */
class TestMockPriorityOutputPort: public TestMockOutputPort {
  public:
    TestMockPriorityOutputPort(AbstractDevice *parent, unsigned int port_id):
      TestMockOutputPort(parent, port_id) {}
  protected:
    bool SupportsPriorities() const { return true; }
};


/*
 * A mock device
 */
class MockDevice: public ola::Device {
  public:
    MockDevice(ola::AbstractPlugin *owner, const string &name):
      Device(owner, name) {}
    string DeviceId() const { return Name(); }
    bool AllowLooping() const { return false; }
    bool AllowMultiPortPatching() const { return false; }
};


/*
 * A mock device with looping and multiport patching enabled
 */
class MockDeviceLoopAndMulti: public ola::Device {
  public:
    MockDeviceLoopAndMulti(ola::AbstractPlugin *owner, const string &name):
      Device(owner, name) {}
    string DeviceId() const { return Name(); }
    bool AllowLooping() const { return true; }
    bool AllowMultiPortPatching() const { return true; }
};
/*
 * A mock plugin.
 */
class TestMockPlugin: public ola::Plugin {
  public:
    explicit TestMockPlugin(const ola::PluginAdaptor *plugin_adaptor):
      Plugin(plugin_adaptor) {}
    string Name() const { return "foo"; }
    string Description() const { return "bar"; }
    ola::ola_plugin_id Id() const { return ola::OLA_PLUGIN_ALL; }
    string PluginPrefix() const { return "test"; }
};
#endif  // OLAD_TESTCOMMON_H_
