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
 * STLUtils.h
 * Helper functions for dealing with the STL.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_STL_STLUTILS_H_
#define INCLUDE_OLA_STL_STLUTILS_H_

#include <map>
#include <vector>

namespace ola {

using std::map;
using std::vector;

/**
 * Returns true if the container contains the value
 */
template<typename T1, typename T2>
inline bool STLContains(const T1 &container, const T2 &value) {
  return container.find(value) != container.end();
}

/**
 * For a vector of pointers, loop through and delete all of them.
 */
template<typename T>
void STLDeleteValues(vector<T*> &values) {
  typename vector<T*>::iterator iter = values.begin();
  for (; iter != values.end(); ++iter)
    delete *iter;
  values.clear();
}

/**
 * For a map of type : pointer, loop through and delete all of the pointers.
 */
template<typename T1, typename T2>
void STLDeleteValues(map<T1, T2*> &values) {
  typename map<T1, T2*>::iterator iter = values.begin();
  for (; iter != values.end(); ++iter)
    delete iter->second;
  values.clear();
}
}  // ola
#endif  // INCLUDE_OLA_STL_STLUTILS_H_
