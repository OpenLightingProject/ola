/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * STLUtils.h
 * Helper functions for dealing with the STL.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_STL_STLUTILS_H_
#define INCLUDE_OLA_STL_STLUTILS_H_

#include <cstddef>
#include <map>
#include <set>
#include <vector>

namespace ola {

using std::map;
using std::set;
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
void STLDeleteValues(vector<T*> *values) {
  typename vector<T*>::iterator iter = values->begin();
  for (; iter != values->end(); ++iter)
    delete *iter;
  values->clear();
}


/**
 * Same as above but for a set. We duplicate the code so that we're sure the
 * argument is a set of pointers, rather than any type with begin() and end()
 * defined.
 */
template<typename T>
void STLDeleteValues(set<T*> *values) {
  typename set<T*>::iterator iter = values->begin();
  for (; iter != values->end(); ++iter)
    delete *iter;
  values->clear();
}


// Helper functions for associative containers.
//------------------------------------------------------------------------------

/**
 * For a map of type : pointer, loop through and delete all of the pointers.
 */
template<typename T1>
void STLDeleteValues(T1 *container) {
  typename T1::iterator iter = container->begin();
  for (; iter != container->end(); ++iter)
    delete iter->second;
  container->clear();
}


/**
 * Extract a vector of keys from an associative container.
 */
template<typename T1>
void STLKeys(const T1 &container, vector<typename T1::key_type> *keys) {
  typename T1::iterator iter = container.begin();
  for (; iter != container.end(); ++iter)
    keys->push_back(iter->first);
}

/**
 * Extract a vector of values from an associative container.
 */
template<typename T1>
void STLValues(const T1 &container, vector<typename T1::mapped_type> *values) {
  typename T1::const_iterator iter = container.begin();
  for (; iter != container.end(); ++iter)
    values->push_back(iter->second);
}


/**
 * Lookup a value by key in a associative container, or return a NULL if the
 * key doesn't exist.
 */
template<typename T1>
typename T1::mapped_type STLFindOrNull(const T1 &container,
                                       const typename T1::key_type &key) {
  typename T1::const_iterator iter = container.find(key);
  if (iter == container.end()) {
    return NULL;
  } else {
    return iter->second;
  }
}


/**
 * Sets key : value, replacing any existing value. Note if value_type is a
 * pointer, you probably don't wait this since you'll leak memory if you
 * replace a value. Use STLSafeReplace instead.
 */
template<typename T1>
void STLReplace(T1 *container, const typename T1::key_type &key,
                const typename T1::mapped_type &value) {
  std::pair<typename T1::iterator, bool> p = container->insert(
      typename T1::value_type(key, value));
  if (!p.second) {
    p.first->second = value;
  }
}


/**
 * Insert an key : value into a map where value is a pointer. This replaces the
 * previous value if it exists and deletes it.
 */
template<typename T1>
void STLSafeReplace(T1 *container, const typename T1::key_type &key,
                    const typename T1::mapped_type &value) {
  std::pair<typename T1::iterator, bool> p = container->insert(
      typename T1::value_type(key, value));
  if (!p.second) {
    delete p.first.second;
    p.first.second = value;
  }
}


/**
 * Insert an key : value into a map only if a value for this key doesn't
 * already exist.
 * Returns true if the key was inserted, false if the key already exists.
 */
template<typename T1>
bool STLInsertIfNotPresent(T1 *container, const typename T1::key_type &key,
                           const typename T1::mapped_type &value) {
  return container->insert(typename T1::value_type(key, value)).second;
}
}  // ola
#endif  // INCLUDE_OLA_STL_STLUTILS_H_
