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
 * PidStore.cpp
 * The PidStore and Pid classes.
 * Copyright (C) 2011 Simon Newton
 */

#include <string>
#include <vector>

#include "common/rdm/PidStoreLoader.h"
#include "ola/StringUtils.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace rdm {

using std::string;
using std::vector;

RootPidStore::~RootPidStore() {
  m_esta_store.reset();
  STLDeleteValues(&m_manufacturer_store);
}

const PidStore *RootPidStore::ManufacturerStore(uint16_t esta_id) const {
  ManufacturerMap::const_iterator iter = m_manufacturer_store.find(esta_id);
  if (iter == m_manufacturer_store.end())
    return NULL;
  return iter->second;
}

const PidDescriptor *RootPidStore::GetDescriptor(
    const string &pid_name) const {
  string canonical_pid_name = pid_name;
  ola::ToUpper(&canonical_pid_name);
  return InternalESTANameLookup(canonical_pid_name);
}

const PidDescriptor *RootPidStore::GetDescriptor(
    const string &pid_name,
    uint16_t manufacturer_id) const {
  string canonical_pid_name = pid_name;
  ola::ToUpper(&canonical_pid_name);

  const PidDescriptor *descriptor = InternalESTANameLookup(canonical_pid_name);
  if (descriptor)
    return descriptor;

  // now try the specific manufacturer store
  const PidStore *store = ManufacturerStore(manufacturer_id);
  if (store)
    return store->LookupPID(canonical_pid_name);
  return NULL;
}

const PidDescriptor *RootPidStore::GetDescriptor(uint16_t pid_value) const {
  if (m_esta_store.get()) {
    const PidDescriptor *descriptor = m_esta_store->LookupPID(pid_value);
    if (descriptor)
      return descriptor;
  }
  return NULL;
}

const PidDescriptor *RootPidStore::GetDescriptor(
    uint16_t pid_value,
    uint16_t manufacturer_id) const {
  const PidDescriptor *descriptor = GetDescriptor(pid_value);
  if (descriptor)
    return descriptor;

  // now try the specific manufacturer store
  const PidStore *store = ManufacturerStore(manufacturer_id);
  if (store) {
    const PidDescriptor *descriptor = store->LookupPID(pid_value);
    if (descriptor)
      return descriptor;
  }
  return NULL;
}

const PidDescriptor *RootPidStore::InternalESTANameLookup(
    const string &canonical_pid_name) const {
  if (m_esta_store.get()) {
    const PidDescriptor *descriptor = m_esta_store->LookupPID(
        canonical_pid_name);
    if (descriptor)
      return descriptor;
  }
  return NULL;
}

const RootPidStore *RootPidStore::LoadFromFile(const string &file,
                                               bool validate) {
  PidStoreLoader loader;
  return loader.LoadFromFile(file, validate);
}

const RootPidStore *RootPidStore::LoadFromDirectory(
    const string &directory,
    bool validate) {
  PidStoreLoader loader;
  string data_source = directory;
  if (directory.empty()) {
    data_source = DataLocation();
  }
  return loader.LoadFromDirectory(data_source, validate);
}

const string RootPidStore::DataLocation() {
  // Provided at compile time.
  return PID_DATA_DIR;
}

PidStore::PidStore(const vector<const PidDescriptor*> &pids) {
  vector<const PidDescriptor*>::const_iterator iter = pids.begin();
  for (; iter != pids.end(); ++iter) {
    m_pid_by_value[(*iter)->Value()] = *iter;
    m_pid_by_name[(*iter)->Name()] = *iter;
  }
}

PidStore::~PidStore() {
  STLDeleteValues(&m_pid_by_value);
  m_pid_by_name.clear();
}

void PidStore::AllPids(vector<const PidDescriptor*> *pids) const {
  pids->reserve(pids->size() + m_pid_by_value.size());

  PidMap::const_iterator iter = m_pid_by_value.begin();
  for (; iter != m_pid_by_value.end(); ++iter) {
    pids->push_back(iter->second);
  }
}


/**
 * Clean up
 */
PidDescriptor::~PidDescriptor() {
  delete m_get_request;
  delete m_get_response;
  delete m_set_request;
  delete m_set_response;
}


/**
 * Lookup a PID by value.
 * @param pid_value the 16 bit pid value.
 */
const PidDescriptor *PidStore::LookupPID(uint16_t pid_value) const {
  PidMap::const_iterator iter = m_pid_by_value.find(pid_value);
  if (iter == m_pid_by_value.end())
    return NULL;
  else
    return iter->second;
}


/**
 * Lookup a PID by name
 * @param pid_name the name of the pid.
 */
const PidDescriptor *PidStore::LookupPID(const string &pid_name) const {
  PidNameMap::const_iterator iter = m_pid_by_name.find(pid_name);
  if (iter == m_pid_by_name.end())
    return NULL;
  else
    return iter->second;
}


/**
 * Check if a GET request to this subdevice is valid
 * @param sub_device the sub device for this request.
 * @returns true if the request is valid, false otherwise.
 */
bool PidDescriptor::IsGetValid(uint16_t sub_device) const {
  return m_get_request && RequestValid(sub_device, m_get_subdevice_range);
}


/**
 * Check if a SET request to this subdevice is valid
 * @param sub_device the sub device for this request.
 * @returns true if the request is valid, false otherwise.
 */
bool PidDescriptor::IsSetValid(uint16_t sub_device) const {
  return m_set_request && RequestValid(sub_device, m_set_subdevice_range);
}


/**
 * @brief Compare PIDs by name
 *
 * Suitable for use as a sort comparison function
 */
bool PidDescriptor::OrderByName(const PidDescriptor *a,
                                const PidDescriptor *b) {
  return a->Name() < b->Name();
}


/**
 * Returns is a request is valid
 */
bool PidDescriptor::RequestValid(uint16_t sub_device,
                                 const sub_device_validator &validator) const {
  switch (validator) {
    case ROOT_DEVICE:  // 0 only
      return sub_device == 0;
    case ANY_SUB_DEVICE:  // 0 - 512 of ALL_RDM_SUBDEVICES
      return (sub_device <= MAX_SUBDEVICE_NUMBER ||
              sub_device == ALL_RDM_SUBDEVICES);
    case NON_BROADCAST_SUB_DEVICE:  // 0 - 512
      return sub_device <= MAX_SUBDEVICE_NUMBER;
    case SPECIFIC_SUB_DEVICE:  // 1 - 512
      return sub_device > 0 && sub_device <= MAX_SUBDEVICE_NUMBER;
    default:
      return false;
  }
}
}  // namespace rdm
}  // namespace ola
