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
 * E131TestFramework.h
 * Allows testing of a remote E1.31 implementation.
 * Copyright (C) 2010 Simon Newton
 *
 * The remote node needs to be listening for Universe 1.
 */

#ifndef PLUGINS_E131_E131_E131TESTFRAMEWORK_H_
#define PLUGINS_E131_E131_E131TESTFRAMEWORK_H_

#include <termios.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/DmxBuffer.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/E131Node.h"

using ola::DmxBuffer;
using ola::network::SelectServer;
using ola::plugin::e131::CID;
using ola::plugin::e131::E131Node;
using std::cout;
using std::endl;
using std::string;

static const unsigned int UNIVERSE_ID = 1;


/*
 * NodeAction, this reflects an action to be performed on a node.
 */
class NodeAction {
  public:
    virtual ~NodeAction() {}
    void SetNode(E131Node *node) { m_node = node; }
    virtual void Tick() {}
  protected:
    E131Node *m_node;
};


/*
 *  A TestState, this represents a particular state of the testing engine. This
 *  one specifies the behaviour of two nodes.
 */
class TestState {
  public:
    TestState(const string &name,
              NodeAction *action1,
              NodeAction *action2,
              const string &expected):
        m_name(name),
        m_expected(expected),
        m_action1(action1),
        m_action2(action2) {
    }
    ~TestState() {
      delete m_action1;
      delete m_action2;
    }

    void SetNodes(E131Node *node1, E131Node *node2) {
      m_action1->SetNode(node1);
      m_action2->SetNode(node2);
    }

    void Tick() {
      m_action1->Tick();
      m_action2->Tick();
    }

    string StateName() const { return m_name; }
    string ExpectedResults() const { return m_expected; }

  protected:
    string m_name, m_expected;
    NodeAction *m_action1, *m_action2;
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
 * This action just sends some data wil the selected priority.
 */
class NodeSimpleSend: public NodeAction {
  public:
    NodeSimpleSend(uint8_t priority, const string &data = ""):
        m_priority(priority) {
      if (data.empty())
        m_buffer.SetRangeToValue(0, m_priority, DMX_UNIVERSE_SIZE);
      else
        m_buffer.SetFromString(data);
    }
    void Tick() {
      m_node->SendDMX(UNIVERSE_ID, m_buffer, m_priority);
    }

  private:
    DmxBuffer m_buffer;
    uint8_t m_priority;
};


/*
 * This action sends a terminated msg the does nothing.
 */
class NodeTerminate: public NodeAction {
  public:
    NodeTerminate():
        m_sent(false) {
    }
    void Tick() {
      if (!m_sent)
        m_node->StreamTerminated(UNIVERSE_ID);
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
        DmxBuffer output;
        output.SetRangeToValue(0, m_data, DMX_UNIVERSE_SIZE);
        m_node->StreamTerminated(UNIVERSE_ID, output);
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
      srand((unsigned)time(0) * getpid());
    }

    void Tick() {
      int random = (rand() / (RAND_MAX / m_chance));
      if (!m_counter || random % m_chance) {
        // start off with good data
        DmxBuffer output;
        output.SetRangeToValue(0, m_good, DMX_UNIVERSE_SIZE);
        m_node->SendDMX(UNIVERSE_ID, output);
      } else {
        // fake an old packet
        DmxBuffer output;
        output.SetRangeToValue(0, m_bad, DMX_UNIVERSE_SIZE);
        int offset = (rand() / (RAND_MAX / 19));
        m_node->SendDMXWithSequenceOffset(UNIVERSE_ID, output, -offset);
      }
      m_counter++;
    }
  private:
    unsigned int m_counter, m_chance;
    uint8_t m_good, m_bad;
};


class StateManager {
  public:
    explicit StateManager(const std::vector<TestState*> &states):
        m_node1(NULL),
        m_node2(NULL),
        m_ss(NULL),
        m_states(states),
        m_stdin_socket(STDIN_FILENO),
        m_count(0),
        m_ticker(0) {
    }
    ~StateManager();
    bool Init();
    void Run() { m_ss->Run(); }

    int Tick();
    int Input();

  private:
    termios m_old_tc;
    CID m_cid1, m_cid2;
    E131Node *m_node1, *m_node2;
    SelectServer *m_ss;
    std::vector<TestState*> m_states;
    ola::network::UnmanagedSocket m_stdin_socket;
    unsigned int m_count, m_ticker;

    void EnterState(TestState *state);
    void NextState();
};
#endif  // PLUGINS_E131_E131_E131TESTFRAMEWORK_H_
