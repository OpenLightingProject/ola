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

#include <assert.h>
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
using ola::network::UdpSocket;
using ola::plugin::e131::CID;
using ola::plugin::e131::E131Node;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;


bool StateManager::Init() {
  m_cid = CID::Generate();
  m_ss = new SelectServer();

  for (TestState **iter = m_states; *iter; iter++)
    m_total_states++;

  // setup the node
  m_node = new E131Node("", m_cid);
  assert(m_node->Start());
  assert(m_ss->AddSocket(m_node->GetSocket()));
  m_node->SetSourceName(UNIVERSE_ID, "E1.31 Merge Test");

  // setup notifications for stdin
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

  cout << "========= E1.31 Tester ==========" << endl;
  cout << "Space for the next state, 'e' for expected results, 'q' to quit"
    << endl;

  EnterState(*m_states);
  return true;
}


StateManager::~StateManager() {
  tcsetattr(STDIN_FILENO, TCSANOW, &m_old_tc);
  assert(m_ss->RemoveSocket(m_node->GetSocket()));
  delete m_ss;
  delete m_node;
}


int StateManager::Tick() {
  m_ticker++;
  (*m_states)->Tick();
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
      cout << (*m_states)->ExpectedResults() << endl;
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
  if (state) {
    cout << "------------------------------------" << endl;
    cout << "Test Case: " << static_cast<int>(m_count) << "/" <<
      static_cast<int>(m_total_states) << endl;
    cout << "Test Name: " << (*m_states)->StateName() << endl;
    state->SetNode(m_node);
  }
}


void StateManager::NextState() {
  m_states++;
  m_count++;
  if (!*m_states) {
    cout << "------------------------------------" << endl;
    cout << "Tests complete!" << endl;
    m_ss->Terminate();
    return;
  }
  EnterState(*m_states);
  return;
}
