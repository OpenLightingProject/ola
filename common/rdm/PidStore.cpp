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
 * PidStore.cpp
 * The PidStore and Pid classes.
 * Copyright (C) 2011 Simon Newton
 */

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <string>
#include <vector>
#include "ola/rdm/PidStore.h"
#include "ola/rdm/RDMEnums.h"
#include "common/rdm/Pids.pb.h"


namespace ola {
namespace rdm {

using std::map;
using std::vector;

/**
 * Lookup a PidStore based on Manufacturer Id
 * @param esta_id the manufacturer id
 * @returns A pointer to a PidStore or NULL if not found
 */
const PidStore *RootPidStore::ManufacturerStore(uint16_t esta_id) const {
  ManufacturerMap::const_iterator iter = m_manufacturer_store.find(esta_id);
  if (iter == m_manufacturer_store.end())
    return NULL;
  return iter->second;
}

/**
 * Load Pid information from a file.
 * @param file the path to the file to load
 * @param validate set to true if we should perform validation of the contents.
 * @returns true if loaded ok, false otherwise.
 */
bool RootPidStore::LoadFromFile(const string &file, bool validate) {
  ola::rdm::pid::PidStore pid_store_pb;
  int fd = 0;
  google::protobuf::io::FileInputStream input_stream(fd);
  bool ok = google::protobuf::TextFormat::Parse(&input_stream, &pid_store_pb);

  return ok;
  (void) file;
  (void) validate;
  return true;
}


/**
 * Load Pid information from a string
 * @param file the path to the file to load
 * @param validate set to true if we should perform validation of the contents.
 * @returns true if loaded ok, false otherwise.
 */
bool RootPidStore::LoadFromStream(std::istream *data, bool validate) {
  ola::rdm::pid::PidStore pid_store_pb;
  google::protobuf::io::IstreamInputStream input_stream(data);
  bool ok = google::protobuf::TextFormat::Parse(&input_stream, &pid_store_pb);

  return ok;
  (void) data;
  (void) validate;
}


/**
 * Create a new PidStore
 * @param a list of PidDescriptors for this store.
 */
PidStore::PidStore(const vector<const PidDescriptor*> &pids) {
  vector<const PidDescriptor*>::const_iterator iter = pids.begin();
  for (; iter != pids.end(); ++iter) {
    m_pid_by_value[(*iter)->Value()] = *iter;
    m_pid_by_name[(*iter)->Name()] = *iter;
  }
}


/**
 * Return a list of all pids
 * @param a pointer to a vector in which to put the PidDescriptors.
 */
void PidStore::AllPids(vector<const PidDescriptor*> *pids) const {
  pids->reserve(m_pid_by_value.size());

  PidMap::const_iterator iter = m_pid_by_value.begin();
  for (; iter != m_pid_by_value.end(); ++iter) {
    pids->push_back(iter->second);
  }
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


bool PidDescriptor::RequestValid(uint16_t sub_device,
                                 const sub_device_valiator &validator) const {
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
}  // rdm
}  // ola
