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
 * VariableInterpolator.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Logging.h>
#include <string>
#include "tools/ola_trigger/VariableInterpolator.h"

using std::string;


/**
 * Interpolate variables within the Context input the input string.
 */
bool InterpolateVariables(const string &input,
                          string *output,
                          const Context &context) {
  static const char START_VARIABLE_STRING[] = "${";
  static const char END_VARIABLE_STRING[] = "}";
  static const char ESCAPE_CHARACTER = '\\';

  *output = input;

  size_t pos = output->size();
  while (true) {
    pos = output->rfind(START_VARIABLE_STRING, pos);
    if (pos == string::npos)
      break;

    // found a ${
    if (pos != 0 && (*output)[pos - 1] == ESCAPE_CHARACTER) {
      // not a real ${
      pos = pos - 1;
      continue;
    }

    // search forward for the closing brace
    size_t closing = output->find(END_VARIABLE_STRING, pos);
    if (closing == string::npos) {
      // not found
      OLA_WARN << "Variable expansion failed for " << *output << ", missing "
        << END_VARIABLE_STRING << " after character " << pos;
      return false;
    }

    // lookup
    size_t variable_name_pos = pos + sizeof(START_VARIABLE_STRING) - 1;
    const string variable = output->substr(variable_name_pos,
                                           closing - variable_name_pos);
    string value;
    if (!context.Lookup(variable, &value)) {
      OLA_WARN << "Unknown variable " << variable;
      return false;
    }
    output->replace(pos, closing - pos + 1, value);
  }

  // finally unescape any braces
  for (unsigned i = 0; i < output->size(); i++) {
    char c = (*output)[i];
    if (c == START_VARIABLE_STRING[0] || c == END_VARIABLE_STRING[0]) {
      if (i != 0 && (*output)[i - 1] == ESCAPE_CHARACTER)
        output->erase(i - 1, 1);
    }
  }
  return true;
}
