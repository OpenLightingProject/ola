/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * ActionQueue.h
 * Interface for the ActionQueue class.
 * Copyright (C) 2005 Simon Newton
 *
 * An ActionQueue allows Actions to be queued up and the executed
 * asynchronously. Perform(on_done) is called on each action in turn and an
 * action calls on_done when it's completed.
 */

#ifndef INCLUDE_OLA_ACTIONQUEUE_H_
#define INCLUDE_OLA_ACTIONQUEUE_H_

#include <ola/Callback.h>
#include <vector>

namespace ola {

/*
 * The base action class
 */
class Action {
 public:
    Action() {}
    virtual ~Action() {}

    // returns true if the failure of this action causes aborts the entire
    // chain.
    virtual bool IsFatal() const = 0;
    virtual bool Failed() const = 0;

    virtual void Perform(SingleUseCallback0<void> *on_done) = 0;
};


/*
 * An Action Queue. Action objects can be added to the queue and are then run
 * sequentially.
 */
class ActionQueue {
 public:
    explicit ActionQueue(SingleUseCallback1<void, ActionQueue*> *on_complete):
      m_on_complete(on_complete),
      m_action_index(-1),
      m_success(true) {
    }
    ~ActionQueue();

    void AddAction(Action *action);
    void NextAction();
    bool WasSuccessful() const { return m_success; }

    unsigned int ActionCount() const { return m_actions.size(); }
    Action *GetAction(unsigned int i);

 private:
    SingleUseCallback1<void, ActionQueue*> *m_on_complete;
    std::vector<Action*> m_actions;
    int m_action_index;
    bool m_success;
};
}  // namespace ola
#endif  // INCLUDE_OLA_ACTIONQUEUE_H_
