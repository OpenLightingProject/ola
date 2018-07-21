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
 * Action.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Logging.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <vector>

#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <ola/win/CleanWindows.h>
#include <tchar.h>
#endif  // _WIN32

#include <ola/stl/STLUtils.h>
#include "tools/ola_trigger/Action.h"
#include "tools/ola_trigger/VariableInterpolator.h"

using std::string;
using std::vector;


/**
 * @brief Assign the value to the variable.
 */
void VariableAssignmentAction::Execute(Context *context, uint8_t) {
  string interpolated_value;
  bool ok = InterpolateVariables(m_value, &interpolated_value, *context);

  if (ok) {
    if (context) {
      OLA_INFO << "Setting " << m_variable << " to \"" << interpolated_value
               << "\"";
      context->Update(m_variable, interpolated_value);
    }
  } else {
    OLA_WARN << "Failed to expand variables in " << m_value;
  }
}


/**
 * @brief Execute the command
 */
void CommandAction::Execute(Context *context, uint8_t) {
  char **args = BuildArgList(context);

  if (ola::LogLevel() >= ola::OLA_LOG_INFO) {
    std::ostringstream str;
    char **ptr = args;
    str << "Executing: " << m_command << " : [";
    ptr++;  // skip over argv[0]
    while (*ptr) {
      str << "\"" << *ptr++ << "\"";
      if (*ptr) {
        str << ", ";
      }
    }
    str << "]";
    OLA_INFO << str.str();
  }

#ifdef _WIN32
  std::ostringstream command_line_builder;
  char** arg = args;
  // Escape argv[0] if needed
  if ((m_command.find(" ") != string::npos) &&
      (m_command.find("\"") != 0)) {
      command_line_builder << "\"" << m_command << "\" ";
  } else {
    command_line_builder << m_command << " ";
  }
  ++arg;
  while (*arg) {
    command_line_builder << " " << *arg++;
  }

  STARTUPINFO startup_info;
  PROCESS_INFORMATION process_information;

  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);
  memset(&process_information, 0, sizeof(process_information));

  LPTSTR cmd_line = _strdup(command_line_builder.str().c_str());

  if (!CreateProcessA(NULL,
                     cmd_line,
                     NULL,
                     NULL,
                     FALSE,
                     CREATE_NEW_CONSOLE,
                     NULL,
                     NULL,
                     &startup_info,
                     &process_information)) {
    OLA_WARN << "Could not launch " << args[0] << ": " << GetLastError();
    FreeArgList(args);
  } else {
    // Don't leak the handles
    CloseHandle(process_information.hProcess);
    CloseHandle(process_information.hThread);
  }

  free(cmd_line);
#else
  pid_t pid;
  if ((pid = fork()) < 0) {
    OLA_FATAL << "Could not fork to exec " << m_command;
    FreeArgList(args);
    return;
  } else if (pid) {
    // parent
    OLA_DEBUG << "Child for " << m_command << " is " << pid;
    FreeArgList(args);
    return;
  }

  execvp(m_command.c_str(), args);
#endif  // _WIN32
}


/**
 * Interpolate all the arguments, and return a pointer to an array of char*
 * pointers which can be passed to exec()
 */
char **CommandAction::BuildArgList(const Context *context) {
  // we need to add the command here as the first arg, also +1 for the NULL
  unsigned int array_size = m_arguments.size() + 2;
  char **args = new char*[array_size];
  memset(args, 0, sizeof(args[0]) * array_size);

  args[0] = StringToDynamicChar(m_command);

  vector<string>::const_iterator iter = m_arguments.begin();
  unsigned int i = 1;
  for (; iter != m_arguments.end(); i++, iter++) {
    string result;
    if (!InterpolateVariables(*iter, &result, *context)) {
      FreeArgList(args);
      return NULL;
    }
    args[i] = StringToDynamicChar(result);
  }
  return args;
}


/**
 * @brief Free the arg array.
 */
void CommandAction::FreeArgList(char **args) {
  char **ptr = args;
  while (*ptr) {
    delete[] *ptr++;
  }
  delete[] args;
}


/**
 * @brief Build a null terminated char* on the heap from a string.
 * @param str the string object to convert
 * @returns a new[] char with the contents of the string.
 */
char *CommandAction::StringToDynamicChar(const string &str) {
  unsigned int str_size = str.size() + 1;
  char *s = new char[str_size];
  strncpy(s, str.c_str(), str_size);
  return s;
}


/**
 * @brief Return the interval as a string.
 */
string ValueInterval::AsString() const {
  std::ostringstream str;
  if (m_lower == m_upper) {
    str << static_cast<int>(m_lower);
  } else {
    str << "[" << static_cast<int>(m_lower) << ", "
        << static_cast<int>(m_upper) << "]";
  }
  return str.str();
}


/**
 * @brief Stream operator
 */
std::ostream& operator<<(std::ostream &out, const ValueInterval &i) {
  return out << i.AsString();
}


/**
 * @brief Cleanup
 */
Slot::~Slot() {
  ActionVector::iterator iter = m_actions.begin();
  for (; iter != m_actions.end(); iter++) {
    delete iter->interval;
  }
  m_actions.clear();

  if (m_default_rising_action) {
    m_default_rising_action->DeRef();
  }
  if (m_default_falling_action) {
    m_default_falling_action->DeRef();
  }
}


/**
 * @brief Attempt to associated an Action with a interval
 * @param lower_value the lower bound of the interval
 * @param upper_value the upper bound of the interval
 * @param action the Action to take if the value is contained within this
 *   interval.
 * @returns true if the interval was added, false otherwise.
 */
bool Slot::AddAction(const ValueInterval &interval_arg,
                     Action *rising_action,
                     Action *falling_action) {
  ActionInterval action_interval(
      new ValueInterval(interval_arg),
      rising_action,
      falling_action);

  if (m_actions.empty()) {
    m_actions.push_back(action_interval);
    return true;
  }

  ActionVector::iterator lower = m_actions.begin();
  if (IntervalsIntersect(action_interval.interval, lower->interval)) {
    delete action_interval.interval;
    return false;
  }

  if (*(action_interval.interval) < *(lower->interval)) {
    m_actions.insert(lower, action_interval);
    return true;
  }

  ActionVector::iterator upper = m_actions.end();
  upper--;
  if (IntervalsIntersect(action_interval.interval, upper->interval)) {
    delete action_interval.interval;
    return false;
  }

  if (*(upper->interval) < *(action_interval.interval)) {
    // action_interval goes at the end
    m_actions.insert(m_actions.end(), action_interval);
    return true;
  }

  if (lower == upper) {
    OLA_WARN << "Inconsistent interval state, adding "
             << *(action_interval.interval) << ", to "
             << IntervalsAsString(m_actions.begin(), m_actions.end());
    delete action_interval.interval;
    return false;
  }

  /**
   * We need to insert the interval between the lower and upper
   * @pre lower != upper
   * @pre the new interval falls between lower and upper
   * @pre lower and upper don't intersect with the new interval
   */
  while (true) {
    if (lower + 1 == upper) {
      // the new interval goes between the two
      m_actions.insert(upper, action_interval);
      return true;
    }

    unsigned int difference = upper - lower;
    ActionVector::iterator mid = lower + difference / 2;

    if (IntervalsIntersect(action_interval.interval, mid->interval)) {
      delete action_interval.interval;
      return false;
    }

    if (*(action_interval.interval) < *(mid->interval)) {
      upper = mid;
    } else if (*(mid->interval) < *(action_interval.interval)) {
      lower = mid;
    } else {
      OLA_WARN << "Inconsistent intervals detected when inserting: "
               << *(action_interval.interval) << ", intervals: "
               << IntervalsAsString(lower, upper);
      delete action_interval.interval;
      return false;
    }
  }
  return true;
}


/**
 * @brief Set the default rising action.
 *
 * If a default already exists this replaces it.
 * @param action the action to install as the default
 * @returns true if there was already a default action.
 */
bool Slot::SetDefaultRisingAction(Action *action) {
  return SetDefaultAction(&m_default_rising_action, action);
}


/**
 * @brief Set the default falling action.
 *
 * If a default already exists this replaces it.
 * @param action the action to install as the default
 * @returns true if there was already a default action.
 */
bool Slot::SetDefaultFallingAction(Action *action) {
  return SetDefaultAction(&m_default_falling_action, action);
}


/**
 * @brief Lookup the action for a value, and if we find one, execute it. Otherwise
 *   execute the default action if there is one.
 * @param context the Context to use
 * @param value the value to look up.
 */
void Slot::TakeAction(Context *context,
                      uint8_t value) {
  if (m_old_value_defined && value == m_old_value) {
    // nothing to do
    return;
  }

  // set the context correctly
  if (context) {
    context->SetSlotOffset(m_slot_offset + 1);
    context->SetSlotValue(value);
  }

  bool rising = true;
  if (m_old_value_defined) {
    rising = value > m_old_value;
  }

  Action *action = LocateMatchingAction(value, rising);
  if (action) {
    action->Execute(context, value);
  } else {
    if (rising && m_default_rising_action) {
      m_default_rising_action->Execute(context, value);
    } else if (!rising && m_default_falling_action) {
      m_default_falling_action->Execute(context, value);
    }
  }

  m_old_value_defined = true;
  m_old_value = value;
}


/**
 * @brief Return the intervals as a string, useful for debugging
 * @returns the intervals as a string.
 */
string Slot::IntervalsAsString() const {
  return IntervalsAsString(m_actions.begin(), m_actions.end());
}


/**
 * @brief Given two interval iterators, first and last, return true if the value is
 *   contained within the lower and upper bounds of the intervals.
 */
bool Slot::ValueWithinIntervals(uint8_t value,
                                const ValueInterval &lower_interval,
                                const ValueInterval &upper_interval) {
  return lower_interval.Lower() <= value && value <= upper_interval.Upper();
}


/**
 * @brief Check if two ValueIntervals intersect.
 */
bool Slot::IntervalsIntersect(const ValueInterval *a1,
                              const ValueInterval *a2) {
  if (a1->Intersects(*a2)) {
    OLA_WARN << "Interval " << *a1 << " overlaps " << *a2;
    return true;
  }
  return false;
}


/**
 * @brief Given a value, find the matching ValueInterval.
 * @param value the value to search for
 * @param rising, true if the new value is rising, false otherwise.
 * @returns the Action matching the value,  or NULL if there isn't one.
 */
Action *Slot::LocateMatchingAction(uint8_t value, bool rising) {
  if (m_actions.empty()) {
    return NULL;
  }

  ActionVector::iterator lower = m_actions.begin();
  ActionVector::iterator upper = m_actions.end();
  upper--;
  if (!ValueWithinIntervals(value, *(lower->interval), *(upper->interval))) {
    return NULL;
  }

  // ok, we know the value lies between the intervals we have, first exclude
  // the endpoints
  if (lower->interval->Contains(value)) {
    return rising ? lower->rising_action : lower->falling_action;
  }

  if (upper->interval->Contains(value)) {
    return rising ? upper->rising_action : upper->falling_action;
  }

  // value isn't at the lower or upper interval, but lies somewhere between
  // the two.
  // @pre lower != upper
  // @pre !lower.Contains(value) && !upper.Contains(value)
  while (true) {
    unsigned int difference = upper - lower;
    ActionVector::iterator mid = lower + difference / 2;

    if (mid == lower) {
      // lower doesn't contain the value, so we don't have it
      return NULL;
    }

    if (mid->interval->Contains(value)) {
      return rising ? mid->rising_action : mid->falling_action;
    } else if (value <= mid->interval->Lower()) {
      upper = mid;
    } else if (value >= mid->interval->Upper()) {
      lower = mid;
    } else {
      OLA_WARN << "Inconsistent intervals detected when looking for: "
               << static_cast<int>(value) << ", intervals: "
               << IntervalsAsString(lower, upper);
      return NULL;
    }
  }
}


/**
 * @brief Format the intervals between the two iterators as a string
 * @param start an iterator pointing to the first interval
 * @param end an iterator pointing to the last interval
 * @return a string version of the intervals.
 */
string Slot::IntervalsAsString(const ActionVector::const_iterator &start,
                               const ActionVector::const_iterator &end) const {
  ActionVector::const_iterator iter = start;
  std::ostringstream str;
  for (; iter != end; ++iter) {
    if (iter != start) {
      str << ", ";
    }
    str << *(iter->interval);
  }
  return str.str();
}


/**
 * @brief Set one of the default actions.
 */
bool Slot::SetDefaultAction(Action **action_to_set,
                            Action *new_action) {
  bool previous_default_set = false;
  new_action->Ref();

  if (*action_to_set) {
    previous_default_set = true;
    (*action_to_set)->DeRef();
  }
  *action_to_set = new_action;
  return previous_default_set;
}
