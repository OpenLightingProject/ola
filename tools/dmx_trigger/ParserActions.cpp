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
 * ParserActions.cpp
 * Copyright (C) 2011 Simon Newton
 *
 * These functions all called by the parser.
 */

#include <ola/Logging.h>
#include <string>
#include <vector>
#include "tools/dmx_trigger/ParserActions.h"

using std::string;
using std::vector;


/**
 * Set the default value of a variable
 * @param input a two element vector in the form [variable_name, value]
 */
void SetDefaultValue(vector<string> *input) {
  if (input->size() != 2) {
    OLA_INFO << "Assignment incorrect";
    exit(1);
  }

  // change the context
  OLA_INFO << "Setting " << (*input)[0] << " = !" <<
      (*input)[1] << "!";

  // delete the pointer since we're done
  delete input;
}


/**
 * Create a new VariableAssignmentAction.
 * @param input a two element vector in the form [variable_name, value]
 * @returns a VariableAssignmentAction object
 */
Action *CreateAssignmentAction(vector<string> *input) {
  OLA_INFO << "Creating assignment action: " << (*input)[0] << " = " <<
      (*input)[1];

  delete input;
  return NULL;
}


Action *CreateCommandAction(const string &command, vector<string> *input) {
  OLA_INFO << "Creating command action: " << command;
  OLA_INFO << "  Args:";
  for (unsigned int i = 0; i < input->size(); i++)
    OLA_INFO << "    " << (*input)[i];
  delete input;
  return NULL;
}


/**
 * Associate an action with a set of values on a particular slot
 * @param slot the slot offset
 * @param slot_values a vector of 
 * @param action the Action object to use
 */
void SetSlotAction(unsigned int slot,
                   vector<ValueInterval*> *slot_values,
                   Action *action) {
  OLA_INFO << "Channel " << slot << ", action " << action;
  (void) slot_values;
}


/**
 * Set the default action for a slot
 * @param slot the slot offset
 * @param action the action to use as the default
 */
void SetDefaultAction(unsigned int slot, Action *action) {
  OLA_INFO << "Channel " << slot << " default action is " << action;
  // lookup SlotActions given the channel
}
