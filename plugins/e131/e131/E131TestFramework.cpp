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
 * E131TestFramework.cpp
 * Allows testing of a remote E1.31 implementation.
 * Copyright (C) 2010 Simon Newton
 *
 * The remote node needs to be listening for Universe 1.
 */

#include <termios.h>
#include <unistd.h>
#include <string>
#include "ola/BaseTypes.h"
#include "ola/Closure.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/e131/e131/CID.h"
#include "plugins/e131/e131/E131Node.h"
#include "plugins/e131/e131/E131TestFramework.h"


using ola::DmxBuffer;
using ola::network::SelectServer;
using ola::plugin::e131::CID;
using ola::plugin::e131::E131Node;
using std::cout;
using std::endl;
using std::string;


bool StateManager::Init() {
  m_cid1 = CID::Generate();
  m_cid2 = CID::Generate();
  m_ss = new SelectServer();

  // setup the node
  m_node1 = new E131Node("", m_cid1);
  m_node2 = new E131Node("", m_cid2, false, true, 5569);
  assert(m_node1->Start());
  assert(m_node2->Start());
  assert(m_ss->AddSocket(m_node1->GetSocket()));
  assert(m_ss->AddSocket(m_node2->GetSocket()));
  m_node1->SetSourceName(UNIVERSE_ID, "E1.31 Merge Test Node 1");
  m_node2->SetSourceName(UNIVERSE_ID, "E1.31 Merge Test Node 2");

  // setup notifications for stdin & turn off buffering
  m_stdin_socket.SetOnData(ola::NewClosure(this, &StateManager::Input));
  m_ss->AddSocket(&m_stdin_socket);
  tcgetattr(STDIN_FILENO, &m_old_tc);
  termios new_tc = m_old_tc;
  new_tc.c_lflag &= (~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tc);

  // tick every 200ms
  m_ss->RegisterRepeatingTimeout(
      200,
      ola::NewClosure(this, &StateManager::Tick));

  cout << endl;
  cout << "========= E1.31 Tester ==========" << endl;
  cout << "Space for the next state, 'e' for expected results, 'q' to quit"
    << endl;

  EnterState(m_states[0]);
  return true;
}


StateManager::~StateManager() {
  tcsetattr(STDIN_FILENO, TCSANOW, &m_old_tc);
  assert(m_ss->RemoveSocket(m_node1->GetSocket()));
  assert(m_ss->RemoveSocket(m_node2->GetSocket()));
  delete m_ss;
  delete m_node1;
  delete m_node2;
}


int StateManager::Tick() {
  m_ticker++;
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
  cout << static_cast<char>(0x8) << std::flush;
  return 0;
}


int StateManager::Input() {
  switch (getchar()) {
    case 'e':
      cout << m_states[m_count]->ExpectedResults() << endl;
      break;
    case 'q':
      m_ss->Terminate();
      break;
    case ' ':
      NextState();
      break;
    default:
      break;
  }
  return 0;
}


void StateManager::EnterState(TestState *state) {
  cout << "------------------------------------" << endl;
  cout << "Test Case: " << static_cast<int>(m_count + 1) << "/" <<
    m_states.size() << endl;
  cout << "Test Name: " << state->StateName() << endl;
  state->SetNodes(m_node1, m_node2);
}


void StateManager::NextState() {
  m_count++;
  if (m_count == m_states.size()) {
    cout << "------------------------------------" << endl;
    cout << "Tests complete!" << endl;
    m_ss->Terminate();
  } else {
    EnterState(m_states[m_count]);
  }
}
