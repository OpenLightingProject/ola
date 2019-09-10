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
 * Action.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef TOOLS_OLA_TRIGGER_ACTION_H_
#define TOOLS_OLA_TRIGGER_ACTION_H_

#include <stdint.h>
#include <ola/Logging.h>
#include <sstream>
#include <string>
#include <vector>

#include "tools/ola_trigger/Context.h"

/*
 * @brief An Action is a behavior that is run when a particular DMX value is received
 * on a particular slot.
 *
 * This is the base class that all others inherit from.
 */
class Action {
 public:
  Action()
    : m_ref_count(0) {
  }
  virtual ~Action() {}

  void Ref() {
    m_ref_count++;
  }
  void DeRef() {
    m_ref_count--;
    if (m_ref_count == 0) {
      delete this;
    }
  }
  virtual void Execute(Context *context, uint8_t slot_value) = 0;

 private:
  unsigned int m_ref_count;
};


/**
 * @brief An Action that assigned a value to a variable
 */
class VariableAssignmentAction: public Action {
 public:
  VariableAssignmentAction(const std::string &variable,
                           const std::string &value)
      : Action(),
        m_variable(variable),
        m_value(value) {
  }

  void Execute(Context *context, uint8_t slot_value);

 private:
  const std::string m_variable;
  const std::string m_value;
};


/**
 * @brief Command Action. This action executes a command.
 */
class CommandAction: public Action {
 public:
  CommandAction(const std::string &command,
                const std::vector<std::string> &arguments)
      : m_command(command),
        m_arguments(arguments) {
  }
  virtual ~CommandAction() {}

  virtual void Execute(Context *context, uint8_t slot_value);

 protected:
  const std::string m_command;
  std::vector<std::string> m_arguments;

  char **BuildArgList(const Context *context);
  void FreeArgList(char **args);
  char *StringToDynamicChar(const std::string &str);
};


/**
 * @brief An interval of DMX values
 */
class ValueInterval {
 public:
  ValueInterval(uint8_t lower, uint8_t upper)
      : m_lower(lower),
        m_upper(upper) {
  }
  ~ValueInterval() {}

  uint8_t Lower() const { return m_lower; }
  uint8_t Upper() const { return m_upper; }

  bool Contains(uint8_t value) const {
    return value >= m_lower && value <= m_upper;
  }

  bool Intersects(const ValueInterval &other) const {
    return (other.Contains(m_lower) || other.Contains(m_upper) ||
            Contains(other.m_lower) || Contains(other.m_upper));
  }

  bool operator<(const ValueInterval &other) const {
    return m_lower < other.m_lower;
  }

  std::string AsString() const;
  friend std::ostream& operator<<(std::ostream &out, const ValueInterval&);

 private:
  uint8_t m_lower, m_upper;
};


/**
 * @brief The set of intervals and their actions.
 */
class Slot {
 public:
  explicit Slot(uint16_t slot_offset)
    : m_default_rising_action(NULL),
      m_default_falling_action(NULL),
      m_slot_offset(slot_offset),
      m_old_value(0),
      m_old_value_defined(false) {
  }
  ~Slot();

  void SetSlotOffset(uint16_t offset) { m_slot_offset = offset; }
  uint16_t SlotOffset() const { return m_slot_offset; }

  bool AddAction(const ValueInterval &interval,
                 Action *rising_action,
                 Action *falling_action);
  bool SetDefaultRisingAction(Action *action);
  bool SetDefaultFallingAction(Action *action);
  void TakeAction(Context *context, uint8_t value);

  std::string IntervalsAsString() const;

  bool operator<(const Slot &other) const {
    return m_slot_offset < other.m_slot_offset;
  }

 private:
  Action *m_default_rising_action;
  Action *m_default_falling_action;
  uint16_t m_slot_offset;
  uint8_t m_old_value;
  bool m_old_value_defined;

  /**
   * @brief An interval of DMX values and the action to be taken for matching
   * values.
   */ 
  class ActionInterval {
   public:
    ActionInterval(const ValueInterval *interval,
                   Action *rising_action,
                   Action *falling_action)
        : interval(interval),
          rising_action(rising_action),
          falling_action(falling_action) {
      if (rising_action) {
        rising_action->Ref();
      }
      if (falling_action) {
        falling_action->Ref();
      }
    }

    ActionInterval(const ActionInterval &other)
        : interval(other.interval),
          rising_action(other.rising_action),
          falling_action(other.falling_action) {
      if (rising_action) {
        rising_action->Ref();
      }
      if (falling_action) {
        falling_action->Ref();
      }
    }

    ~ActionInterval() {
      if (rising_action) {
        rising_action->DeRef();
      }
      if (falling_action) {
        falling_action->DeRef();
      }
    }

    ActionInterval &operator=(const ActionInterval &other) {
      if (this != &other) {
        interval = other.interval;

        if (rising_action) {
          rising_action->DeRef();
        }
        rising_action = other.rising_action;
        if (rising_action) {
          rising_action->Ref();
        }

        if (falling_action) {
          falling_action->DeRef();
        }
        falling_action = other.falling_action;
        if (falling_action) {
          falling_action->Ref();
        }
      }
      return *this;
    }

    const ValueInterval *interval;
    Action *rising_action;
    Action *falling_action;
  };

  typedef std::vector<ActionInterval> ActionVector;
  ActionVector m_actions;

  bool ValueWithinIntervals(uint8_t value,
                            const ValueInterval &lower_interval,
                            const ValueInterval &upper_interval);
  bool IntervalsIntersect(const ValueInterval *a1,
                          const ValueInterval *a2);
  Action *LocateMatchingAction(uint8_t value, bool rising);
  std::string IntervalsAsString(const ActionVector::const_iterator &start,
                                const ActionVector::const_iterator &end) const;
  bool SetDefaultAction(Action **action_to_set, Action *new_action);
};
#endif  // TOOLS_OLA_TRIGGER_ACTION_H_
