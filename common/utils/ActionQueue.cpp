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
 * OlaHttpServer.cpp
 * Ola HTTP class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <vector>

#include "ola/ActionQueue.h"
#include "ola/Callback.h"
#include "ola/Logging.h"


namespace ola {

using std::vector;

ActionQueue::~ActionQueue() {
  vector<Action*>::const_iterator iter;
  for (iter = m_actions.begin(); iter != m_actions.end(); ++iter)
    delete *iter;
  m_actions.clear();
}


void ActionQueue::AddAction(Action *action) {
  m_actions.push_back(action);
}


/*
 * Check the state of the current action, and if necessary run the next action.
 */
void ActionQueue::NextAction() {
  if (!m_success)
    return;

  if (m_action_index >= 0 && m_action_index <
      static_cast<int>(m_actions.size())) {
    if (m_actions[m_action_index]->IsFatal() &&
        m_actions[m_action_index]->Failed()) {
      // abort the chain here
      m_success = false;
      m_on_complete->Run(this);
      return;
    }
  }

  if (m_action_index >= static_cast<int>(m_actions.size())) {
    OLA_WARN << "Action queue already finished!";
  } else if (m_action_index == static_cast<int>(m_actions.size()) - 1) {
    m_action_index++;
    m_on_complete->Run(this);
  } else {
    m_action_index++;
    m_actions[m_action_index]->Perform(
        NewSingleCallback(this, &ActionQueue::NextAction));
  }
}

Action *ActionQueue::GetAction(unsigned int i) {
  if (i >= ActionCount())
    return NULL;
  return m_actions[i];
}
}  // ola
