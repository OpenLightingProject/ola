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
 * e131_merge_test.cpp
 * The sends custom E1.31 packets in order to stress test the merge
 * implementation of a remote node.
 * Copyright (C) 2010 Simon Newton
 *
 * The remote node needs to be listening for Universe 1.
 */

#ifndef PLUGINS_E131_E131_E131TESTFRAMEWORK_H_
#define PLUGINS_E131_E131_E131TESTFRAMEWORK_H_

#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <string>
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
using std::stringstream;

static const unsigned int UNIVERSE_ID = 1;

/*
 * The base test state class
 */
class TestState {
  public:
    void SetNode(E131Node *node) { m_node = node; }
    virtual ~TestState() {}
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void Tick() {}
    virtual string StateName() const = 0;
    virtual string ExpectedResults() const = 0;
  protected:
    E131Node *m_node;
};


/*
 * This state just sends some data wil the selected priority.
 */
class SimpleSendState: public TestState {
  public:
    explicit SimpleSendState(uint8_t priority):
        m_priority(priority) {
    }
    void Tick() {
      DmxBuffer output;
      output.SetRangeToValue(0, m_priority, DMX_UNIVERSE_SIZE);
      m_node->SendDMX(UNIVERSE_ID, output, m_priority);
    }
    string StateName() const { return "Simple Send"; }
    string ExpectedResults() const {
      stringstream str;
      str << "512 values of " << static_cast<int>(m_priority);
      return str.str();
    }

  private:
    uint8_t m_priority;
};


/*
 * This state does nothing.
 */
class TimeoutState: public TestState {
  public:
    TimeoutState() {}
    void Tick() {}
    string StateName() const { return "Timeout"; }
    string ExpectedResults() const { return "Nothing"; }
};


class StateManager {
  public:
    explicit StateManager(TestState **states):
        m_node(NULL),
        m_ss(NULL),
        m_states(states),
        m_stdin_socket(STDIN_FILENO),
        m_count(1),
        m_total_states(0),
        m_ticker(0) {
    }
    ~StateManager();
    bool Init();
    void Run() { m_ss->Run(); }

    int Tick();
    int Input();

  private:
    termios m_old_tc;
    CID m_cid;
    E131Node *m_node;
    SelectServer *m_ss;
    TestState **m_states;
    ola::network::UnmanagedSocket m_stdin_socket;
    unsigned int m_count, m_total_states, m_ticker;

    void EnterState(TestState *state);
    void NextState();
};
#endif  // PLUGINS_E131_E131_E131TESTFRAMEWORK_H_
