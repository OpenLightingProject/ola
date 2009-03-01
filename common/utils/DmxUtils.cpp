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
 * DmxUtils.cpp
 * Random Dmx functions.
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <vector>
#include <lla/DmxUtils.h>
#include <lla/StringUtils.h>

namespace lla {

using std::string;
using std::vector;

/*
 * Convert a ',' separated list into a dmx_t array. Invalid values are set to
 * 0. 0s can be dropped between the commas.
 * @param input the string to split
 * @param dmx_data the array to store the dmx values in
 * @param length the the array
 * @returns the length of dmx data written
 */
unsigned int StringToDmx(const string &input, dmx_t *dmx_data,
                         unsigned int length) {
  unsigned int i = 0;
  vector<string> dmx_values;
  vector<string>::const_iterator iter;

  if (input.empty())
    return 0;
  StringSplit(input, dmx_values, ",");
  for (iter = dmx_values.begin(); iter != dmx_values.end() && i < length;
       ++iter, ++i) {
    dmx_data[i] = atoi(iter->data());
  }
  return i;
}

} //lla
