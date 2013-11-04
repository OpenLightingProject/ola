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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * HttpServerActions.h
 * The list of actions the Ola Server performs.
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef OLAD_HTTPSERVERACTIONS_H_
#define OLAD_HTTPSERVERACTIONS_H_

#include <stdint.h>
#include <string>
#include "ola/ActionQueue.h"
#include "ola/OlaCallbackClient.h"
#include "ola/base/Macro.h"

namespace ola {

using std::string;


/*
 * The base action
 */
class BaseHttpAction: public Action {
  public:
    explicit BaseHttpAction(OlaCallbackClient *client):
      Action(),
      m_client(client),
      m_failed(false),
      m_on_done(NULL) {
    }
    virtual ~BaseHttpAction() {}

    bool Failed() const  { return m_failed; }
    void Perform(SingleUseCallback0<void> *on_done);
    void CallbackComplete(const string &error);

  protected:
    OlaCallbackClient *m_client;

    void RequestComplete(bool failure);
    virtual bool DoAction() = 0;

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
    SetNameAction(OlaCallbackClient *client,
                  unsigned int universe,
                  const string &name,
                  bool is_fatal):
      BaseHttpAction(client),
      m_universe(universe),
      m_name(name),
      m_is_fatal(is_fatal) {
    }

    bool IsFatal() const { return m_is_fatal; }

  protected:
    bool DoAction();

  private:
    unsigned int m_universe;
    string m_name;
    bool m_is_fatal;

    DISALLOW_COPY_AND_ASSIGN(SetNameAction);
};


/*
 * An action that sets the merge mode of a universe
 */
class SetMergeModeAction: public BaseHttpAction {
  public:
    SetMergeModeAction(OlaCallbackClient *client,
                       unsigned int universe,
                       OlaUniverse::merge_mode mode):
      BaseHttpAction(client),
      m_universe(universe),
      m_merge_mode(mode) {
    }

    bool IsFatal() const { return false; }

  protected:
    bool DoAction();

  private:
    unsigned int m_universe;
    OlaUniverse::merge_mode m_merge_mode;

    DISALLOW_COPY_AND_ASSIGN(SetMergeModeAction);
};


/*
 * An action that adds or removes a port from a universe.
 */
class PatchPortAction: public BaseHttpAction {
  public:
    PatchPortAction(OlaCallbackClient *client,
                  unsigned int device_alias,
                  unsigned int port,
                  PortDirection direction,
                  unsigned int universe,
                  PatchAction action):
      BaseHttpAction(client),
      m_device_alias(device_alias),
      m_port(port),
      m_direction(direction),
      m_universe(universe),
      m_action(action) {
    }

    bool IsFatal() const { return false; }

  protected:
    bool DoAction();

  private:
    unsigned int m_device_alias;
    unsigned int m_port;
    PortDirection m_direction;
    unsigned int m_universe;
    PatchAction m_action;

    DISALLOW_COPY_AND_ASSIGN(PatchPortAction);
};


/*
 * An action that sets a port priority to inherit mode.
 */
class PortPriorityInheritAction: public BaseHttpAction {
  public:
    PortPriorityInheritAction(OlaCallbackClient *client,
                              unsigned int device_alias,
                              unsigned int port,
                              PortDirection direction):
      BaseHttpAction(client),
      m_device_alias(device_alias),
      m_port(port),
      m_direction(direction) {
    }

    bool IsFatal() const { return false; }

  protected:
    bool DoAction();

  private:
    unsigned int m_device_alias;
    unsigned int m_port;
    PortDirection m_direction;

    DISALLOW_COPY_AND_ASSIGN(PortPriorityInheritAction);
};


/*
 * An action that sets a port priority to override mode.
 */
class PortPriorityStaticAction: public BaseHttpAction {
  public:
    PortPriorityStaticAction(OlaCallbackClient *client,
                              unsigned int device_alias,
                              unsigned int port,
                              PortDirection direction,
                              uint8_t overide_value):
      BaseHttpAction(client),
      m_device_alias(device_alias),
      m_port(port),
      m_direction(direction),
      m_override_value(overide_value) {
    }

    bool IsFatal() const { return false; }

  protected:
    bool DoAction();

  private:
    unsigned int m_device_alias;
    unsigned int m_port;
    PortDirection m_direction;
    uint8_t m_override_value;

    DISALLOW_COPY_AND_ASSIGN(PortPriorityStaticAction);
};
}  // namespace ola
#endif  // OLAD_HTTPSERVERACTIONS_H_
