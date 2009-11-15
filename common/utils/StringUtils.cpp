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
 * StringUtils.cpp
 * Random String functions.
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <sstream>
#include <vector>
#include <iostream>
#include <ola/StringUtils.h>

namespace ola {

using std::string;
using std::stringstream;
using std::vector;

/*
 * Split a string on delimtiers. If two delimiters appear next to each other an
 * empty string is added to the output list.
 * @param input the string to split
 * @param tokens the vector to store the result in
 * @param delimiters what to split on
 */
void StringSplit(const string &input,
                 vector<string> &tokens,
                 const string &delimiters) {

  string::size_type start_offset = 0;
  string::size_type end_offset = 0;

  while (1) {
    end_offset = input.find_first_of(delimiters, start_offset);
    if (end_offset == string::npos) {
      tokens.push_back(input.substr(start_offset, input.size() - start_offset));
      return;
    }
    tokens.push_back(input.substr(start_offset, end_offset - start_offset));
    start_offset = end_offset + 1 > input.size() ? string::npos :
                   end_offset + 1;
  }
}


/*
 * Trim leading and trailing whitespace from a string.
 * @param the string to trim
 */
void StringTrim(std::string *input) {
  string characters_to_trim = " \n\r\t";
  string::size_type start = input->find_first_not_of(characters_to_trim);
  string::size_type end = input->find_last_not_of(characters_to_trim);

  if (start == string::npos)
    input->clear();
  else
    *input = input->substr(start, end - start + 1);
}


/*
 * Convert an int to a string.
 * @param i the int to convert
 * @return the string representation of the int
 */
string IntToString(int i) {
  stringstream str;
  str << i;
  return str.str();
}

} //ola
