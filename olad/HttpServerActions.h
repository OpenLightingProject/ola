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
 * HttpServerActions.h
 * The list of actions the Ola Server performs.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef OLAD_HTTPSERVERACTIONS_H_
#define OLAD_HTTPSERVERACTIONS_H_

#include <stdint.h>
#include <string>
#include "ola/ActionQueue.h"
#include "ola/client/OlaClient.h"
#include "ola/base/Macro.h"

namespace ola {

/*
 * The base action
 */
class BaseHttpAction: public Action {
 public:
    explicit BaseHttpAction(client::OlaClient *client):
      Action(),
      m_client(client),
      m_failed(false),
      m_on_done(NULL) {
    }
    virtual ~BaseHttpAction() {}

    bool Failed() const  { return m_failed; }
    void Perform(SingleUseCallback0<void> *on_done);
    void CallbackComplete(const client::Result &result);

 protected:
    client::OlaClient *m_client;

    void RequestComplete(bool failure);
    virtual void DoAction() = 0;

 private:
    bool m_failed;
    SingleUseCallback0<void> *m_on_done;

    DISALLOW_COPY_AND_ASSIGN(BaseHttpAction);
};


/*
 * An action that sets the name of a universe
 */
class SetNameAction: public BaseHttpAction {
 public:
    SetNameAction(client::OlaClient *client,
                  unsigned int universe,
                  const std::string &name,
                  bool is_fatal):
      BaseHttpAction(client),
      m_universe(universe),
      m_name(name),
      m_is_fatal(is_fatal) {
    }

    bool IsFatal() const { return m_is_fatal; }

 protected:
    void DoAction();

 private:
    unsigned int m_universe;
    std::string m_name;
    bool m_is_fatal;

    DISALLOW_COPY_AND_ASSIGN(SetNameAction);
};


/*
 * An action that sets the merge mode of a universe
 */
class SetMergeModeAction: public BaseHttpAction {
 public:
    SetMergeModeAction(client::OlaClient *client,
                       unsigned int universe,
                       client::OlaUniverse::merge_mode mode):
      BaseHttpAction(client),
      m_universe(universe),
      m_merge_mode(mode) {
    }

    bool IsFatal() const { return false; }

 protected:
    void DoAction();

 private:
    unsigned int m_universe;
    client::OlaUniverse::merge_mode m_merge_mode;

    DISALLOW_COPY_AND_ASSIGN(SetMergeModeAction);
};


/*
 * An action that adds or removes a port from a universe.
 */
class PatchPortAction: public BaseHttpAction {
 public:
    PatchPortAction(client::OlaClient *client,
                  unsigned int device_alias,
                  unsigned int port,
                  client::PortDirection direction,
                  unsigned int universe,
                  client::PatchAction action):
      BaseHttpAction(client),
      m_device_alias(device_alias),
      m_port(port),
      m_direction(direction),
      m_universe(universe),
      m_action(action) {
    }

    bool IsFatal() const { return false; }

 protected:
    void DoAction();

 private:
    unsigned int m_device_alias;
    unsigned int m_port;
    client::PortDirection m_direction;
    unsigned int m_universe;
    client::PatchAction m_action;

    DISALLOW_COPY_AND_ASSIGN(PatchPortAction);
};


/*
 * An action that sets a port priority to inherit mode.
 */
class PortPriorityInheritAction: public BaseHttpAction {
 public:
    PortPriorityInheritAction(client::OlaClient *client,
                              unsigned int device_alias,
                              unsigned int port,
                              client::PortDirection direction):
      BaseHttpAction(client),
      m_device_alias(device_alias),
      m_port(port),
      m_direction(direction) {
    }

    bool IsFatal() const { return false; }

 protected:
    void DoAction();

 private:
    unsigned int m_device_alias;
    unsigned int m_port;
    client::PortDirection m_direction;

    DISALLOW_COPY_AND_ASSIGN(PortPriorityInheritAction);
};


/*
 * An action that sets a port priority to override mode.
 */
class PortPriorityStaticAction: public BaseHttpAction {
 public:
    PortPriorityStaticAction(client::OlaClient *client,
                             unsigned int device_alias,
                             unsigned int port,
                             client::PortDirection direction,
                             uint8_t override_value):
      BaseHttpAction(client),
      m_device_alias(device_alias),
      m_port(port),
      m_direction(direction),
      m_override_value(override_value) {
    }

    bool IsFatal() const { return false; }

 protected:
    void DoAction();

 private:
    unsigned int m_device_alias;
    unsigned int m_port;
    client::PortDirection m_direction;
    uint8_t m_override_value;

    DISALLOW_COPY_AND_ASSIGN(PortPriorityStaticAction);
};
}  // namespace ola
#endif  // OLAD_HTTPSERVERACTIONS_H_
