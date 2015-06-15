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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * STLUtils.h
 * Helper functions for dealing with the STL.
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @defgroup stl STL
 * @brief Various helper functions related to STL classes.
 *
 * @addtogroup stl
 * @{
 *
 * @file STLUtils.h
 * @brief Helper functions for STL classes.
 * @}
 */

#ifndef INCLUDE_OLA_STL_STLUTILS_H_
#define INCLUDE_OLA_STL_STLUTILS_H_

#include <assert.h>
#include <cstddef>
#include <map>
#include <set>
#include <vector>
#include <utility>

namespace ola {

/**
 * @addtogroup stl
 * @{
 */

/**
 * @brief Clear a stack.
 * @tparam T A stack.
 * @post The stack is empty.
 */
template<typename T>
void STLEmptyStack(T *stack) {
  while (!stack->empty()) {
    stack->pop();
  }
}

/**
 * @brief Clear a stack and delete all pointers..
 * @tparam T A stack of pointers.
 * @post The stack is empty.
 */
template<typename T>
void STLEmptyStackAndDelete(T *stack) {
  while (!stack->empty()) {
    delete stack->top();
    stack->pop();
  }
}

/**
 * @brief Delete the elements of a Sequence.
 * @param sequence the Sequence to delete the elements from.
 * @tparam T A Sequence.
 * @post The sequence is empty.
 *
 * All elements in the sequence will be deleted. The sequence will be cleared.
 *
 * @examplepara
 * ~~~~~~~~~~~~~~~~~~~~~
 * class Foo() {
 *  public:
 *    Foo() {
 *      // populate m_objects
 *    }
 *    ~Foo() {
 *      STLDeleteElements(&m_objects);
 *    }
 *  private:
 *    vector<Bar*> m_objects;
 * };
 * ~~~~~~~~~~~~~~~~~~~~~
 */
template<typename T>
void STLDeleteElements(T *sequence) {
  typename T::iterator iter = sequence->begin();
  for (; iter != sequence->end(); ++iter) {
    if (*iter) {
      delete *iter;
    }
  }
  sequence->clear();
}

/**
 * Delete all values in a pair associative container.
 * @param container
 * @tparam T A pair associative container.
 * @post The container is empty.
 *
 * All elements in the container will be deleted. The container will be cleared.
 *
 * @examplepara
 * ~~~~~~~~~~~~~~~~~~~~~
 * class Foo() {
 *  public:
 *    Foo() {
 *      // populate m_objects
 *    }
 *    ~Foo() {
 *      STLDeleteValues(&m_objects);
 *    }
 *  private:
 *    map<int, Bar*> m_objects;
 * };
 * ~~~~~~~~~~~~~~~~~~~~~
 * @sa STLDeleteElements.
 */
template<typename T>
void STLDeleteValues(T *container) {
  typename T::iterator iter = container->begin();
  for (; iter != container->end(); ++iter) {
    if (iter->second) {
      delete iter->second;
    }
  }
  container->clear();
}

/**
 * Returns true if the container contains the value.
 * @tparam T1 A container.
 * @tparam T2 The value to search for.
 * @param container the container to search in.
 * @param value the value to search for.
 */
template<typename T1, typename T2>
inline bool STLContains(const T1 &container, const T2 &value) {
  return container.find(value) != container.end();
}

/**
 * Extract the list of keys from a pair associative container.
 * @tparam T1 A container.
 * @param[in] container the container to extract the keys for.
 * @param[out] keys the vector to populate with the keys.
 */
template<typename T1>
void STLKeys(const T1 &container, std::vector<typename T1::key_type> *keys) {
  keys->reserve(keys->size() + container.size());
  typename T1::const_iterator iter = container.begin();
  for (; iter != container.end(); ++iter)
    keys->push_back(iter->first);
}

/**
 * @brief Extract a vector of values from a pair associative container.
 * @tparam T1 A container.
 * @tparam T2 The type of the values.
 * @param[in] container the container to extract the values for.
 * @param[out] values the vector to populate with the values.
 */
template<typename T1, typename T2>
void STLValues(const T1 &container, std::vector<T2> *values) {
  values->reserve(values->size() + container.size());
  typename T1::const_iterator iter = container.begin();
  for (; iter != container.end(); ++iter)
    values->push_back(iter->second);
}

/**
 * @brief Lookup a value by key in a associative container.
 * @tparam T1 A container.
 * @param container the container to search in.
 * @param key the key to search for.
 * @returns A pointer to the value, or NULL if the value isn't found.
 */
template<typename T1>
typename T1::mapped_type* STLFind(T1 *container,
                                  const typename T1::key_type &key) {
  typename T1::iterator iter = container->find(key);
  if (iter == container->end()) {
    return NULL;
  } else {
    return &iter->second;
  }
}

/**
 * @brief Lookup a value by key in a associative container.
 * @tparam T1 A container.
 * @param container the container to search in.
 * @param key the key to search for.
 * @returns A pointer to the value, or NULL if the value isn't found.
 */
template<typename T1>
typename T1::mapped_type const* STLFind(const T1 *container,
                                        const typename T1::key_type &key) {
  typename T1::const_iterator iter = container->find(key);
  if (iter == container->end()) {
    return NULL;
  } else {
    return &iter->second;
  }
}

/**
 * @brief Lookup a value by key in a associative container.
 * @tparam T1 A container.
 * @param container the container to search in.
 * @param key the key to search for.
 * @returns The value matching the key, or NULL if the value isn't found.
 *
 * This assumes that NULL can be co-erced to the mapped_type of the container.
 * It's most suitable for containers with pointers.
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
 * @brief Replace a value in a pair associative container, inserting the key,
 * value if it doesn't already exist.
 * @tparam T1 A container.
 * @param container the container to replace the value in.
 * @param key the key to insert / replace.
 * @param value the value to insert / replace.
 * @returns true if the value was replaced, false if the value was inserted.
 *
 * @note Note if the value type is a pointer, and the container has ownership
 * of the pointer, replacing a value will leak memory. Use STLReplaceAndDelete
 * to avoid this.
 * @sa STLReplaceAndDelete.
 */
template<typename T1>
bool STLReplace(T1 *container, const typename T1::key_type &key,
                const typename T1::mapped_type &value) {
  std::pair<typename T1::iterator, bool> p = container->insert(
      typename T1::value_type(key, value));
  if (!p.second) {
    p.first->second = value;
    return true;
  }
  return false;
}

/**
 * @brief Replace a value in a pair associative container. If the key existed,
 * the old value is returned, otherwise NULL is returned.
 * @tparam T1 A container.
 * @param container the container to replace the value in.
 * @param key the key to insert / replace.
 * @param value the value to insert / replace.
 * @returns The value matching the key, or NULL if the value isn't found.
 *
 * @note This assumes that NULL can be co-erced to the mapped_type of the
 * container. It's most suitable for containers with pointers.
 * @sa STLReplaceAndDelete.
 */
template<typename T1>
typename T1::mapped_type STLReplacePtr(T1 *container,
                                       const typename T1::key_type &key,
                                       const typename T1::mapped_type &value) {
  std::pair<typename T1::iterator, bool> p = container->insert(
      typename T1::value_type(key, value));
  if (!p.second) {
    typename T1::mapped_type old_value = p.first->second;
    p.first->second = value;
    return old_value;
  }
  return NULL;
}


/**
 * @brief Similar to STLReplace but this will delete the value if the
 * replacement occurs.
 * @tparam T1 A container.
 * @param container the container to replace the value in.
 * @param key the key to insert / replace.
 * @param value the value to insert / replace.
 * @returns true if the value was replaced, false if the value was inserted.
 */
template<typename T1>
bool STLReplaceAndDelete(T1 *container, const typename T1::key_type &key,
                         const typename T1::mapped_type &value) {
  std::pair<typename T1::iterator, bool> p = container->insert(
      typename T1::value_type(key, value));
  if (!p.second) {
    delete p.first->second;
    p.first->second = value;
    return true;
  }
  return false;
}


/**
 * @brief Insert a value into a container only if this value doesn't already
 * exist.
 * @tparam T1 A container.
 * @param container the container to insert the value in.
 * @param key_value the pair<key, value> to insert.
 * @returns true if the key/value was inserted, false if the key already exists.
 */
template<typename T1>
bool STLInsertIfNotPresent(T1 *container,
                           const typename T1::value_type &key_value) {
  return container->insert(key_value).second;
}


/**
 * @brief Insert a value into a container only if this value doesn't already
 * exist.
 * @tparam T1 A container.
 * @param container the container to insert the value in.
 * @param key the key to insert.
 * @param value the value to insert.
 * @returns true if the key/value was inserted, false if the key already exists.
 * @sa STLInsertIfNotPresent.
 */
template<typename T1>
bool STLInsertIfNotPresent(T1 *container, const typename T1::key_type &key,
                           const typename T1::mapped_type &value) {
  return container->insert(typename T1::value_type(key, value)).second;
}


/**
 * @brief Insert an key : value into a pair associative container, or abort
 * the program if the key already exists.
 * @tparam T1 A container.
 * @param container the container to insert the value in.
 * @param key the key to insert.
 * @param value the value to insert.
 * @note This should only be used in unit-testing code.
 */
template<typename T1>
void STLInsertOrDie(T1 *container, const typename T1::key_type &key,
                    const typename T1::mapped_type &value) {
  assert(container->insert(typename T1::value_type(key, value)).second);
}


/**
 * @brief Remove a key / value from a container.
 * @tparam T1 A container.
 * @param container the container to remove the key from.
 * @param key the key to remove.
 * @returns true if the item was removed, false otherwise.
 */
template<typename T1>
bool STLRemove(T1 *container, const typename T1::key_type &key) {
  return container->erase(key);
}

/**
 * @brief Lookup and remove a key from a pair associative container.
 * @tparam T1 A container.
 * @param[in] container the container to remove the key from.
 * @param[in] key the key to remove.
 * @param[out] value A pointer which will be set to the removed value.
 * @returns true if the item was found and removed, false otherwise.
 *
 * Lookup a value by key in a pair associative container. If the value exists,
 * it's removed from the container, the value placed in value and true is
 * returned.
 */
template<typename T1>
bool STLLookupAndRemove(T1 *container,
                        const typename T1::key_type &key,
                        typename T1::mapped_type *value) {
  typename T1::iterator iter = container->find(key);
  if (iter == container->end()) {
    return false;
  } else {
    *value = iter->second;
    container->erase(iter);
    return true;
  }
}

/**
 * @brief Lookup or insert a NULL value into a pair associative container.
 * @tparam T1 A container.
 * @param[in] container the container to lookup from or insert into.
 * @param[in] key the key to lookup.
 * @returns An iterator pointing to the value.
 *
 * Lookup a value by key in a pair associative container or insert NULL if it
 * doesn't already exist.
 */
template<typename T1>
typename T1::iterator STLLookupOrInsertNull(T1 *container,
                                            const typename T1::key_type &key) {
  std::pair<typename T1::iterator, bool> p = container->insert(
      typename T1::value_type(key, NULL));
  return p.first;
}

template<typename T1>
void PairAssociativeAssignNew(T1 **location) {
  *location = new T1();
}

/**
 * @brief Lookup or insert a new object into a pair associative container.
 * @tparam T1 A container.
 * @param[in] container the container to lookup from or insert into.
 * @param[in] key the key to lookup.
 * @returns An iterator pointing to the value.
 */
template<typename T1>
typename T1::iterator STLLookupOrInsertNew(T1 *container,
                                           const typename T1::key_type &key) {
  std::pair<typename T1::iterator, bool> p = container->insert(
      typename T1::value_type(key, NULL));
  if (p.second) {
    PairAssociativeAssignNew(&p.first->second);
  }
  return p.first;
}

/**
 * @brief Remove a value from a pair associative container and delete it.
 * @tparam T1 A container.
 * @param[in] container the container to remove the key from.
 * @param[in] key the key to remove.
 * @returns true if the item was found and removed from the container, false
 *   otherwise.
 */
template<typename T1>
bool STLRemoveAndDelete(T1 *container, const typename T1::key_type &key) {
  typename T1::iterator iter = container->find(key);
  if (iter == container->end()) {
    return false;
  } else {
    delete iter->second;
    container->erase(iter);
    return true;
  }
}

/**
 * @brief Remove a value from a pair associative container and return the value.
 * @tparam T1 A container.
 * @param[in] container the container to remove the key from.
 * @param[in] key the key to remove.
 * @returns The value matching the key, or NULL if the value isn't found.
 *
 * @note This assumes that NULL can be co-erced to the mapped_type of the
 * container. It's most suitable for containers with pointers.
 */
template<typename T1>
typename T1::mapped_type STLLookupAndRemovePtr(
    T1 *container,
    const typename T1::key_type &key) {
  typename T1::iterator iter = container->find(key);
  if (iter == container->end()) {
    return NULL;
  } else {
    typename T1::mapped_type value = iter->second;
    container->erase(iter);
    return value;
  }
}

/**
 * Add elements of a sequence to an associative container.
 * @param output The associative container to add to.
 * @param input The sequence containing the elements to add.
 * @param value The value to use for each key in input.
 * @tparam T1 A pair associative container.
 * @tparam T2 A sequence.
 *
 * Any existing elements that conflict with the values in the sequence will be
 * replaced.
 */
template<typename T1, typename T2>
void STLMapFromKeys(T1 *output, const T2 input,
                    typename T1::mapped_type value) {
  typename T2::const_iterator iter = input.begin();
  for (; iter != input.end(); ++iter) {
    std::pair<typename T1::iterator, bool> p = output->insert(
        typename T1::value_type(*iter, value));
    if (!p.second) {
      p.first->second = value;
    }
  }
}
}  // namespace ola
#endif  // INCLUDE_OLA_STL_STLUTILS_H_
/**
 * @}
 */
