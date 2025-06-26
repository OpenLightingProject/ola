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
 * E131TestFramework.cpp
 * Allows testing of a remote E1.31 implementation.
 * Copyright (C) 2010 Simon Newton
 *
 * The remote node needs to be listening for Universe 1.
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "ola/acn/CID.h"
#include "ola/Callback.h"
#include "ola/Constants.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/io/SelectServer.h"
#include "ola/io/StdinHandler.h"
#include "ola/network/Socket.h"
#include "libs/acn/E131Node.h"
#include "libs/acn/E131TestFramework.h"


using ola::DmxBuffer;
using ola::acn::CID;
using ola::io::SelectServer;
using ola::io::StdinHandler;
using ola::acn::E131Node;
using std::cout;
using std::endl;
using std::string;
using std::vector;

StateManager::StateManager(const std::vector<TestState*> &states,
                           bool interactive_mode)
    : m_interactive(interactive_mode),
      m_count(0),
      m_ticker(0),
      m_local_node(NULL),
      m_node1(NULL),
      m_node2(NULL),
      m_ss(NULL),
      m_stdin_handler(nullptr),
      m_states(states) {
}

bool StateManager::Init() {
  m_cid1 = CID::Generate();
  m_cid2 = CID::Generate();
  m_ss = new SelectServer();

  // setup notifications for stdin
  m_stdin_handler.reset(new StdinHandler(
      m_ss, ola::NewCallback(this, &StateManager::Input)));

  if (!m_interactive) {
    // local node test
    CID local_cid = CID::Generate();
    m_local_node = new E131Node(m_ss, "", E131Node::Options(), local_cid);
    assert(m_local_node->Start());
    assert(m_ss->AddReadDescriptor(m_local_node->GetSocket()));

    assert(m_local_node->SetHandler(
          UNIVERSE_ID,
          &m_recv_buffer,
          NULL,  // don't track the priority
          ola::NewCallback(this, &StateManager::NewDMX)));
  }

  E131Node::Options options1;
  options1.port = 5567;
  E131Node::Options options2(options1);
  options1.port = 5569;

  m_node1 = new E131Node(m_ss, "", options1, m_cid1);
  m_node2 = new E131Node(m_ss, "", options2, m_cid2);
  assert(m_node1->Start());
  assert(m_node2->Start());
  assert(m_ss->AddReadDescriptor(m_node1->GetSocket()));
  assert(m_ss->AddReadDescriptor(m_node2->GetSocket()));
  m_node1->SetSourceName(UNIVERSE_ID, "E1.31 Merge Test Node 1");
  m_node2->SetSourceName(UNIVERSE_ID, "E1.31 Merge Test Node 2");

  // tick every 200ms
  m_ss->RegisterRepeatingTimeout(
      TICK_INTERVAL_MS,
      ola::NewCallback(this, &StateManager::Tick));

  cout << endl;
  cout << "========= E1.31 Tester ==========" << endl;
  if (m_interactive) {
    cout << "Space for the next state, 'e' for expected results, 'q' to quit"
         << endl;
  }

  EnterState(m_states[0]);
  return true;
}


StateManager::~StateManager() {
  if (m_node1) {
    m_ss->RemoveReadDescriptor(m_node1->GetSocket());
    delete m_node1;
  }

  if (m_node2) {
    m_ss->RemoveReadDescriptor(m_node2->GetSocket());
    delete m_node2;
  }

  if (m_local_node) {
    m_ss->RemoveReadDescriptor(m_local_node->GetSocket());
    delete m_local_node;
  }

  m_stdin_handler.reset();
  delete m_ss;
}


bool StateManager::Tick() {
  if (m_ticker > (TIME_PER_STATE_MS / TICK_INTERVAL_MS) && !m_interactive) {
    NextState();
    if (m_count == m_states.size()) {
      return false;
    }
  } else {
    m_ticker++;
  }
  m_states[m_count]->Tick();
  switch (m_ticker % 4) {
    case 0:
      cout << "|";
      break;
    case 1:
      cout << "/";
      break;
    case 2:
      cout << "-";
      break;
    case 3:
      cout << "\\";
      break;
  }
  cout << '\b' << std::flush;
  return true;
}


void StateManager::Input(int c) {
  switch (c) {
    case 'e':
      cout << m_states[m_count]->ExpectedResults() << endl;
      break;
    case 'q':
      m_ss->Terminate();
      ShowStatus();
      break;
    case ' ':
      NextState();
      break;
    default:
      break;
  }
}


/*
 * Called when new DMX is received by the local node
 */
void StateManager::NewDMX() {
  if (!m_states[m_count]->Verify(m_recv_buffer)) {
    cout << "FAILED TEST" << endl;
  }
}


/*
 * Switch states
 */
void StateManager::EnterState(TestState *state) {
  cout << "------------------------------------" << endl;
  cout << "Test Case: " << static_cast<int>(m_count + 1) << "/"
       << m_states.size() << endl;
  cout << "Test Name: " << state->StateName() << endl;
  state->SetNodes(m_node1, m_node2);
  m_ticker = 0;
}


void StateManager::NextState() {
  if (!m_states[m_count]->Passed()) {
    m_failed_tests.push_back(m_states[m_count]);
  }

  m_count++;
  if (m_count == m_states.size()) {
    cout << "------------------------------------" << endl;
    cout << "Tests complete!" << endl;
    ShowStatus();
    m_ss->Terminate();
  } else {
    EnterState(m_states[m_count]);
  }
}

void StateManager::ShowStatus() {
  if (!m_failed_tests.empty()) {
    cout << "Some tests failed:" << endl;
    vector<TestState*>::iterator iter;
    for (iter = m_failed_tests.begin(); iter != m_failed_tests.end(); ++iter) {
      cout << "  " << (*iter)->StateName() << endl;
    }
  } else {
    cout << "All tests passed." << endl;
  }
}
