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
 * ParserActions.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * These functions all called by the parser. They modify the globals defined in
 * ParserGlobals.h
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends
#include <ola/Constants.h>
#include <ola/Logging.h>
#include <ola/base/SysExits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "tools/ola_trigger/Action.h"
#include "tools/ola_trigger/ConfigCommon.h"
#include "tools/ola_trigger/Context.h"
#include "tools/ola_trigger/ParserActions.h"
#include "tools/ola_trigger/ParserGlobals.h"

using std::string;
using std::vector;

extern int yylineno;  // defined and maintained in lex.yy.cpp


/**
 * Lookup the Slot objects associated with a slot offset. This creates
 * one if it doesn't already exist.
 * @param slot the slot offset to lookup
 * @return A Slot object
 */
Slot *LookupSlot(uint16_t slot) {
  SlotActionMap::iterator iter = global_slots.find(slot);
  if (iter != global_slots.end())
    return iter->second;

  Slot *actions = new Slot(slot);
  global_slots[slot] = actions;
  return actions;
}

/**
 * Check a slot offset is valid
 */
void CheckSlotOffset(unsigned int slot) {
  if (slot == 0 || slot > ola::DMX_UNIVERSE_SIZE) {
    OLA_FATAL << "Line " << yylineno << ": slot offset " << slot << " invalid";
    exit(ola::EXIT_DATAERR);
  }
}


/**
 * Set the default value of a variable
 * @param input a two element vector in the form [variable_name, value]
 */
void SetDefaultValue(vector<string> *input) {
  if (input->size() != 2) {
    OLA_FATAL << "Line " << yylineno << ": assignment size != 2. Size is " <<
       input->size();
    exit(ola::EXIT_DATAERR);
  }

  global_context->Update((*input)[0], (*input)[1]);

  // delete the pointer since we're done
  delete input;
}


/**
 * Create a new VariableAssignmentAction.
 * @param input a two element vector in the form [variable_name, value]
 * @returns a VariableAssignmentAction object
 */
Action *CreateAssignmentAction(vector<string> *input) {
  if (input->size() != 2) {
    OLA_FATAL << "Line " << yylineno <<
      ": assignment action size != 2. Size is " << input->size();
    exit(ola::EXIT_DATAERR);
  }

  Action *action = new VariableAssignmentAction((*input)[0], (*input)[1]);
  delete input;
  return action;
}


/**
 * Create a new CommandAction.
 * @param command the command to run
 * @param args a list of arguments for the command
 * @returns a CommandAction object
 */
Action *CreateCommandAction(const string &command, vector<string> *args) {
  Action *action = new CommandAction(command, *args);
  delete args;
  return action;
}


/**
 * Create a new ValueInterval object
 * @param lower the lower bound
 * @param upper the upper bound
 * @returns a new ValueInterval object
 */
ValueInterval *CreateInterval(unsigned int lower, unsigned int upper) {
  if (lower > upper) {
    OLA_FATAL << "Line " << yylineno << ": invalid interval " << lower << "-"
        << upper;
    exit(ola::EXIT_DATAERR);
  }
  if (upper > UINT8_MAX) {
    OLA_FATAL << "Line " << yylineno << ": invalid DMX value " << upper;
    exit(ola::EXIT_DATAERR);
  }
  return new ValueInterval(lower, upper);
}


/**
 * Associate an action with a set of values on a particular slot.
 * @param slot the slot offset
 * @param slot_values a vector of ValueIntervals to trigger this action
 * @param action the Action object to use
 */
void SetSlotAction(unsigned int slot,
                   IntervalList *slot_intervals,
                   Action *rising_action,
                   Action *falling_action) {
  CheckSlotOffset(slot);
  slot--;  // convert from 1 to 0 indexed slots
  Slot *slots = LookupSlot(slot);

  IntervalList::iterator iter = slot_intervals->begin();
  for (; iter != slot_intervals->end(); iter++) {
    if (!slots->AddAction(**iter, rising_action, falling_action)) {
      OLA_FATAL << "Line " << yylineno << ": value " << **iter <<
         " collides with existing values.";
      exit(ola::EXIT_DATAERR);
    }
    delete  *iter;
  }
  delete slot_intervals;
}


/**
 * Set the default action for a slot
 * @param slot the slot offset
 * @param action the action to use as the default
 */
void SetDefaultAction(unsigned int slot,
                      Action *rising_action,
                      Action *falling_action) {
  CheckSlotOffset(slot);
  slot--;  // convert from 1 to 0 indexed slots
  Slot *slots = LookupSlot(slot);
  if (rising_action) {
    if (slots->SetDefaultRisingAction(rising_action)) {
      OLA_FATAL << "Multiple default rising actions defined for slot " <<
        slot + 1 << ", line " << yylineno;
      exit(ola::EXIT_DATAERR);
    }
  }
  if (falling_action) {
    if (slots->SetDefaultFallingAction(falling_action)) {
      OLA_FATAL << "Multiple default falling actions defined for slot " <<
        slot + 1 << ", line " << yylineno;
      exit(ola::EXIT_DATAERR);
    }
  }
}
