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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * E131TestFramework.h
 * Allows testing of a remote E1.31 implementation.
 * Copyright (C) 2010 Simon Newton
 *
 * The remote node needs to be listening for Universe 1.
 */

#ifndef LIBS_ACN_E131TESTFRAMEWORK_H_
#define LIBS_ACN_E131TESTFRAMEWORK_H_

#include <unistd.h>
#include <string>
#include <memory>
#include <vector>
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/acn/CID.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"
#include "ola/io/StdinHandler.h"
#include "ola/math/Random.h"
#include "libs/acn/E131Node.h"

static const unsigned int UNIVERSE_ID = 1;

/*
 * NodeAction, this reflects an action to be performed on a node.
 */
class NodeAction {
 public:
    virtual ~NodeAction() {}
    void SetNode(ola::acn::E131Node *node) { m_node = node; }
    virtual void Tick() {}
 protected:
    ola::acn::E131Node *m_node;
};


/*
 *  A TestState, this represents a particular state of the testing engine. This
 *  one specifies the behaviour of two nodes.
 */
class TestState {
 public:
    TestState(const std::string &name,
              NodeAction *action1,
              NodeAction *action2,
              const std::string &expected,
              const ola::DmxBuffer &expected_result):
        m_passed(true),
        m_expected_result(expected_result),
        m_name(name),
        m_expected(expected),
        m_action1(action1),
        m_action2(action2) {
    }
    virtual ~TestState() {
      delete m_action1;
      delete m_action2;
    }

    void SetNodes(ola::acn::E131Node *node1,
                  ola::acn::E131Node *node2) {
      m_action1->SetNode(node1);
      m_action2->SetNode(node2);
    }

    void Tick() {
      m_action1->Tick();
      m_action2->Tick();
    }

    virtual bool Verify(const ola::DmxBuffer &data) {
      if (!(data == m_expected_result))
        return m_passed = false;
      return true;
    }

    std::string StateName() const { return m_name; }
    std::string ExpectedResults() const { return m_expected; }

    bool Passed() const {
      return m_passed;
    }

 protected:
    bool m_passed;
    ola::DmxBuffer m_expected_result;

 private:
    std::string m_name, m_expected;
    NodeAction *m_action1, *m_action2;
};


/*
 * This is similar to a TestState but it checks for a particular first packet.
 * It's useful for state transitions.
 */
class RelaxedTestState: public TestState {
 public:
    RelaxedTestState(const std::string &name,
                     NodeAction *action1,
                     NodeAction *action2,
                     const std::string &expected,
                     const ola::DmxBuffer &expected_first_result,
                     const ola::DmxBuffer &expected_result):
      TestState(name, action1, action2, expected, expected_result),
      m_first(true),
      m_expected_first_result(expected_first_result) {
    }

    bool Verify(const ola::DmxBuffer &buffer) {
      if (m_first) {
        m_first = false;
        if (!(m_expected_first_result == buffer))
          return m_passed = false;
        return true;
      } else {
        if (!(m_expected_result == buffer))
          return m_passed = false;
        return true;
      }
    }

 private:
    bool m_first;
    ola::DmxBuffer m_expected_first_result;
};


/*
 * This is similar to a TestState but it checks for one style of packet,
 * followed by another. It's useful for state transitions.
 */
class OrderedTestState: public TestState {
 public:
    OrderedTestState(const std::string &name,
                     NodeAction *action1,
                     NodeAction *action2,
                     const std::string &expected,
                     const ola::DmxBuffer &expected_first_result,
                     const ola::DmxBuffer &expected_result):
      TestState(name, action1, action2, expected, expected_result),
      m_found_second(false),
      m_expected_first_result(expected_first_result) {
    }

    bool Verify(const ola::DmxBuffer &buffer) {
      if (m_found_second) {
        if (!(m_expected_result == buffer))
          return m_passed = false;
        return true;
      } else {
        if (m_expected_result == buffer) {
          m_found_second = true;
          return true;
        }
        if (!(m_expected_first_result == buffer))
          return m_passed = false;
        return true;
      }
    }

 private:
    bool m_found_second;
    ola::DmxBuffer m_expected_first_result;
};


/*
 * This action does nothing.
 */
class NodeInactive: public NodeAction {
 public:
    NodeInactive() {}
    void Tick() {}
};


/*
 * This action just sends some data with the selected priority.
 */
class NodeSimpleSend: public NodeAction {
 public:
    explicit NodeSimpleSend(uint8_t priority, const std::string &data = "")
        : m_priority(priority) {
      if (data.empty())
        m_buffer.SetRangeToValue(0, m_priority, ola::DMX_UNIVERSE_SIZE);
      else
        m_buffer.SetFromString(data);
    }
    void Tick() {
      m_node->SendDMX(UNIVERSE_ID, m_buffer, m_priority);
    }

 private:
    ola::DmxBuffer m_buffer;
    uint8_t m_priority;
};


/*
 * This action sends a terminated msg that does nothing.
 */
class NodeTerminate: public NodeAction {
 public:
    NodeTerminate():
        m_sent(false) {
    }
    void Tick() {
      if (!m_sent)
        m_node->SendStreamTerminated(UNIVERSE_ID);
      m_sent = true;
    }
 private:
    bool m_sent;
};


/*
 * This state sends a terminated msg with data then does nothing
 */
class NodeTerminateWithData: public NodeAction {
 public:
    explicit NodeTerminateWithData(uint8_t data):
        m_data(data),
        m_sent(false) {
    }
    void Tick() {
      if (!m_sent) {
        ola::DmxBuffer output;
        output.SetRangeToValue(0, m_data, ola::DMX_UNIVERSE_SIZE);
        m_node->SendStreamTerminated(UNIVERSE_ID, output);
      }
      m_sent = true;
    }
 private:
    uint8_t m_data;
     bool m_sent;
};


/*
 * This state sends data and occasionally sends old packets to test sequence #
 * handling.
 */
class NodeVarySequenceNumber: public NodeAction {
 public:
    NodeVarySequenceNumber(uint8_t good_value, uint8_t bad_value,
                           unsigned int chance):
        m_counter(0),
        m_chance(chance),
        m_good(good_value),
        m_bad(bad_value) {
      ola::math::InitRandom();
    }

    void Tick() {
      int random = ola::math::Random(0, static_cast<int>(m_chance) - 1);
      if (!m_counter || random) {
        // start off with good data
        ola::DmxBuffer output;
        output.SetRangeToValue(0, m_good, ola::DMX_UNIVERSE_SIZE);
        m_node->SendDMX(UNIVERSE_ID, output);
      } else {
        // fake an old packet, 1 to 18 packets behind.
        ola::DmxBuffer output;
        output.SetRangeToValue(0, m_bad, ola::DMX_UNIVERSE_SIZE);
        int offset = ola::math::Random(1, 18);
        m_node->SendDMXWithSequenceOffset(UNIVERSE_ID, output,
                                          static_cast<int8_t>(-offset));
      }
      m_counter++;
    }

 private:
    unsigned int m_counter, m_chance;
    uint8_t m_good, m_bad;
};


/*
 * The state manager can run in one of three modes:
 *  - local, non-interactive. This starts a local E131Node and sends it data,
 *  verifying against the expected output.
 *  - interactive mode. This sends data to the multicast addresses and a human
 *  gets to verify it.
 */
class StateManager {
 public:
    StateManager(const std::vector<TestState*> &states,
                 bool interactive_mode = false);
    ~StateManager();
    bool Init();
    void Run() { m_ss->Run(); }
    bool Tick();
    void Input(int c);
    void NewDMX();
    bool Passed() const { return m_failed_tests.empty(); }

 private:
    bool m_interactive;
    unsigned int m_count, m_ticker;
    ola::acn::CID m_cid1, m_cid2;
    ola::acn::E131Node *m_local_node, *m_node1, *m_node2;
    ola::io::SelectServer *m_ss;
    std::auto_ptr<ola::io::StdinHandler> m_stdin_handler;
    std::vector<TestState*> m_states;
    ola::DmxBuffer m_recv_buffer;
    std::vector<TestState*> m_failed_tests;

    void EnterState(TestState *state);
    void NextState();
    void ShowStatus();

    static const unsigned int TICK_INTERVAL_MS = 100;
    static const unsigned int TIME_PER_STATE_MS = 3000;
};
#endif  // LIBS_ACN_E131TESTFRAMEWORK_H_
