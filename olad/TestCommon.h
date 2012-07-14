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
#include <memory>
#include <string>
#include <vector>

#include "ola/Clock.h"
#include "ola/DmxBuffer.h"
#include "ola/rdm/RDMCommand.h"
#include "ola/rdm/UIDSet.h"
#include "olad/Device.h"
#include "olad/Plugin.h"
#include "olad/Port.h"
#include "olad/PortManager.h"

using ola::AbstractDevice;
using ola::BasicInputPort;
using ola::BasicOutputPort;
using ola::DmxBuffer;
using ola::TimeStamp;
using ola::rdm::UIDSet;
using std::string;
using std::vector;


/*
 * Mock out an Input Port
 */
class TestMockInputPort: public BasicInputPort {
  public:
    TestMockInputPort(AbstractDevice *parent,
                      unsigned int port_id,
                      const ola::PluginAdaptor *plugin_adaptor):
      BasicInputPort(parent, port_id, plugin_adaptor) {}
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
    TestMockPriorityInputPort(AbstractDevice *parent,
                              unsigned int port_id,
                              const ola::PluginAdaptor *plugin_adaptor):
        TestMockInputPort(parent, port_id, plugin_adaptor),
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
class TestMockOutputPort: public BasicOutputPort {
  public:
    TestMockOutputPort(AbstractDevice *parent,
                       unsigned int port_id,
                       bool start_rdm_discovery_on_patch = false,
                       bool supports_rdm = false):
      BasicOutputPort(parent, port_id, start_rdm_discovery_on_patch,
                      supports_rdm) {
    }
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
 * Mock out an RDM OutputPort
 */
class TestMockRDMOutputPort: public TestMockOutputPort {
  public:
    typedef ola::BaseCallback2<void,
                               const ola::rdm::RDMRequest*,
                               ola::rdm::RDMCallback*> RDMRequestHandler;

    TestMockRDMOutputPort(AbstractDevice *parent,
                          unsigned int port_id,
                          UIDSet *uids,
                          bool start_rdm_discovery_on_patch = false,
                          RDMRequestHandler *rdm_handler = NULL)
        : TestMockOutputPort(parent,
                             port_id,
                             start_rdm_discovery_on_patch,
                             true),
          m_uids(uids),
          m_rdm_handler(rdm_handler) {
    }
    ~TestMockRDMOutputPort() {}

    void SetRDMHandler(RDMRequestHandler *handler) {
      m_rdm_handler.reset(handler);
    }

    void SendRDMRequest(const ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback) {
      // if a RDMRequestHandler was provided use that.
      if (m_rdm_handler.get()) {
        m_rdm_handler->Run(request, callback);
        return;
      }

      // otherwise just return a RDM_FAILED_TO_SEND
      vector<string> packets;
      delete request;
      callback->Run(ola::rdm::RDM_FAILED_TO_SEND, NULL, packets);
    }

    void RunFullDiscovery(
        ola::rdm::RDMDiscoveryCallback *on_complete) {
      on_complete->Run(*m_uids);
    }

    void RunIncrementalDiscovery(
        ola::rdm::RDMDiscoveryCallback *on_complete) {
      on_complete->Run(*m_uids);
    }

  private:
    UIDSet *m_uids;
    std::auto_ptr<RDMRequestHandler> m_rdm_handler;
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
    explicit TestMockPlugin(ola::PluginAdaptor *plugin_adaptor,
                            ola::ola_plugin_id plugin_id,
                            bool should_start = true):
      Plugin(plugin_adaptor),
      m_start_run(false),
      m_should_start(should_start),
      m_id(plugin_id) {}
    bool ShouldStart() { return m_should_start; }
    bool StartHook() {
      m_start_run = true;
      return true;
    }
    string Name() const { return "foo"; }
    string Description() const { return "bar"; }
    ola::ola_plugin_id Id() const { return m_id; }
    string PluginPrefix() const { return "test"; }

    bool WasStarted() { return m_start_run; }

  private:
    bool m_start_run;
    bool m_should_start;
    ola::ola_plugin_id m_id;
};


/**
 * We mock this out so we can manipulate the wake up time. It was either this
 * or the mocking the plugin adaptor.
 */
class MockSelectServer: public ola::io::SelectServerInterface {
  public:
    explicit MockSelectServer(const TimeStamp *wake_up):
      SelectServerInterface(),
      m_wake_up(wake_up) {}
    ~MockSelectServer() {}

    bool AddReadDescriptor(ola::io::ReadFileDescriptor *descriptor) {
      (void) descriptor;
      return true;
    }

    bool AddReadDescriptor(ola::io::ConnectedDescriptor *descriptor,
                   bool delete_on_close = false) {
      (void) descriptor;
      (void) delete_on_close;
      return true;
    }

    bool RemoveReadDescriptor(ola::io::ReadFileDescriptor *descriptor) {
      (void) descriptor;
      return true;
    }

    bool RemoveReadDescriptor(ola::io::ConnectedDescriptor *descriptor) {
      (void) descriptor;
      return true;
    }

    bool AddWriteDescriptor(ola::io::WriteFileDescriptor *descriptor) {
      (void) descriptor;
      return true;
    }

    bool RemoveWriteDescriptor(ola::io::WriteFileDescriptor *descriptor) {
      (void) descriptor;
      return true;
    }

    ola::thread::timeout_id RegisterRepeatingTimeout(
        unsigned int,
        ola::Callback0<bool> *) {
      return ola::thread::INVALID_TIMEOUT;
    }
    ola::thread::timeout_id RegisterRepeatingTimeout(
        const ola::TimeInterval&,
        ola::Callback0<bool>*) {
      return ola::thread::INVALID_TIMEOUT;
    }
    ola::thread::timeout_id RegisterSingleTimeout(
        unsigned int,
        ola::SingleUseCallback0<void> *) {
      return ola::thread::INVALID_TIMEOUT;
    }
    ola::thread::timeout_id RegisterSingleTimeout(
        const ola::TimeInterval&,
        ola::SingleUseCallback0<void> *) {
      return ola::thread::INVALID_TIMEOUT;
    }
    void RemoveTimeout(ola::thread::timeout_id id) { (void) id; }
    const TimeStamp *WakeUpTime() const { return m_wake_up; }

    void Execute(ola::BaseCallback0<void> *callback) {
      callback->Run();
    }


  private:
    const TimeStamp *m_wake_up;
};
#endif  // OLAD_TESTCOMMON_H_
