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
 * PidStoreLoader.cpp
 * The PidStoreLoader and helper code.
 * Copyright (C) 2011 Simon Newton
 */

#include <errno.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "common/rdm/DescriptorConsistencyChecker.h"
#include "common/rdm/PidStoreLoader.h"
#include "common/rdm/Pids.pb.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/file/Util.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/stl/STLUtils.h"
#include "ola/strings/Format.h"

namespace ola {
namespace rdm {

using ola::messaging::Descriptor;
using ola::messaging::FieldDescriptor;
using std::auto_ptr;
using std::map;
using std::ostringstream;
using std::set;
using std::string;
using std::vector;

const char PidStoreLoader::OVERRIDE_FILE_NAME[] = "overrides.proto";
const uint16_t PidStoreLoader::ESTA_MANUFACTURER_ID = 0;
const uint16_t PidStoreLoader::MANUFACTURER_PID_MIN = 0x8000;
const uint16_t PidStoreLoader::MANUFACTURER_PID_MAX = 0xffe0;

const RootPidStore *PidStoreLoader::LoadFromFile(const string &file,
                                                 bool validate) {
  std::ifstream proto_file(file.data());

  if (!proto_file.is_open()) {
    OLA_WARN << "Missing " << file << ": " << strerror(errno);
    return NULL;
  }

  const RootPidStore *store = LoadFromStream(&proto_file, validate);
  proto_file.close();
  return store;
}

const RootPidStore *PidStoreLoader::LoadFromDirectory(
    const string &directory,
    bool validate) {
  vector<string> files;

  string override_file;
  vector<string> all_files;
  if (!ola::file::ListDirectory(directory, &all_files)) {
    OLA_WARN << "Failed to list files in " << directory;
    return NULL;
  }
  if (all_files.empty()) {
    OLA_WARN << "Didn't find any files in " << directory;
    return NULL;
  }
  vector<string>::const_iterator file_iter = all_files.begin();
  for (; file_iter != all_files.end(); ++file_iter) {
    if (ola::file::FilenameFromPath(*file_iter) == OVERRIDE_FILE_NAME) {
      override_file = *file_iter;
    } else if (StringEndsWith(*file_iter, ".proto")) {
      files.push_back(*file_iter);
    }
  }
  if (files.empty() && override_file.empty()) {
    OLA_WARN << "Didn't find any files to load in " << directory;
    return NULL;
  }

  ola::rdm::pid::PidStore pid_store_pb;
  vector<string>::const_iterator iter = files.begin();
  for (; iter != files.end(); ++iter) {
    std::ifstream proto_file(iter->data());
    if (!proto_file.is_open()) {
      OLA_WARN << "Failed to open " << *iter << ": " << strerror(errno);
      return NULL;
    }

    google::protobuf::io::IstreamInputStream input_stream(&proto_file);
    bool ok = google::protobuf::TextFormat::Merge(&input_stream,
                                                  &pid_store_pb);
    proto_file.close();

    if (!ok) {
      OLA_WARN << "Failed to load " << *iter;
      return NULL;
    }
  }

  ola::rdm::pid::PidStore override_pb;
  if (!override_file.empty()) {
    if (!ReadFile(override_file, &override_pb)) {
      return NULL;
    }
  }

  return BuildStore(pid_store_pb, override_pb, validate);
}

const RootPidStore *PidStoreLoader::LoadFromStream(std::istream *data,
                                                   bool validate) {
  ola::rdm::pid::PidStore pid_store_pb;
  google::protobuf::io::IstreamInputStream input_stream(data);
  bool ok = google::protobuf::TextFormat::Parse(&input_stream, &pid_store_pb);

  if (!ok)
    return NULL;

  ola::rdm::pid::PidStore override_pb;
  return BuildStore(pid_store_pb, override_pb, validate);
}

bool PidStoreLoader::ReadFile(const std::string &file_path,
                              ola::rdm::pid::PidStore *proto) {
  std::ifstream proto_file(file_path.c_str());
  if (!proto_file.is_open()) {
    OLA_WARN << "Failed to open " << file_path << ": " << strerror(errno);
    return false;
  }

  google::protobuf::io::IstreamInputStream input_stream(&proto_file);
  bool ok = google::protobuf::TextFormat::Merge(&input_stream, proto);
  proto_file.close();

  if (!ok) {
    OLA_WARN << "Failed to load " << file_path;
  }
  return ok;
}

/*
 * Build the RootPidStore from a protocol buffer.
 */
const RootPidStore *PidStoreLoader::BuildStore(
    const ola::rdm::pid::PidStore &store_pb,
    const ola::rdm::pid::PidStore &override_pb,
    bool validate) {
  ManufacturerMap pid_data;
  // Load the overrides first so they get first dibs on each PID.
  if (!LoadFromProto(&pid_data, override_pb, validate)) {
    FreeManufacturerMap(&pid_data);
    return NULL;
  }

  // Load the main data
  if (!LoadFromProto(&pid_data, store_pb, validate)) {
    FreeManufacturerMap(&pid_data);
    return NULL;
  }

  // Now we need to convert the data structure into a format that the PidStore
  // understands.
  auto_ptr<const PidStore> esta_store;
  RootPidStore::ManufacturerMap manufacturer_map;

  ManufacturerMap::iterator iter = pid_data.begin();
  for (; iter != pid_data.end(); ++iter) {
    // Ownership of the Descriptors is transferred to the vector
    vector<const PidDescriptor*> pids;
    STLValues(*iter->second, &pids);
    delete iter->second;

    if (iter->first == ESTA_MANUFACTURER_ID) {
      esta_store.reset(new PidStore(pids));
    } else {
      STLReplaceAndDelete(&manufacturer_map, iter->first,
                          new PidStore(pids));
    }
  }
  pid_data.clear();

  OLA_DEBUG << "Load Complete";
  return new RootPidStore(esta_store.release(),
                          manufacturer_map,
                          store_pb.version());
}

/*
 * @brief Load the data from the PidStore proto into the ManufacturerMap.
 * @param[out] pid_data the ManufacturerMap to populate.
 * @param proto the Protobuf data.
 * @param validate Enables strict validation mode.
 *
 * If a collision occurs, the data in the map is not replaced.
 */
bool PidStoreLoader::LoadFromProto(ManufacturerMap *pid_data,
                                   const ola::rdm::pid::PidStore &proto,
                                   bool validate) {
  set<uint16_t> seen_manufacturer_ids;

  ManufacturerMap::iterator iter = STLLookupOrInsertNew(
      pid_data, ESTA_MANUFACTURER_ID);
  if (!GetPidList(iter->second, proto, validate, true)) {
    return false;
  }

  for (int i = 0; i < proto.manufacturer_size(); ++i) {
    const ola::rdm::pid::Manufacturer &manufacturer = proto.manufacturer(i);

    if (STLContains(seen_manufacturer_ids, manufacturer.manufacturer_id())) {
      OLA_WARN << "Manufacturer id " << manufacturer.manufacturer_id() <<
          "(" << manufacturer.manufacturer_name() <<
          ") listed more than once in the PIDs file";
      return false;
    }
    seen_manufacturer_ids.insert(manufacturer.manufacturer_id());

    ManufacturerMap::iterator iter = STLLookupOrInsertNew(
        pid_data, manufacturer.manufacturer_id());
    if (!GetPidList(iter->second, manufacturer, validate, false)) {
      return false;
    }
  }

  return true;
}

/*
 * @brief Populate a PidMap from a protobuf object that has a set of repeated
 * PIDs.
 */
template <typename pb_object>
bool PidStoreLoader::GetPidList(PidMap *pid_map,
                                const pb_object &store,
                                bool validate,
                                bool limit_pid_values) {
  set<uint16_t> seen_pids;
  set<string> seen_names;

  for (int i = 0; i < store.pid_size(); ++i) {
    const ola::rdm::pid::Pid &pid = store.pid(i);

    OLA_DEBUG << "Loading " << pid.name();
    if (validate) {
      if (STLContains(seen_pids, pid.value())) {
        OLA_WARN << "PID " << pid.value()
                 << " exists multiple times in the pid file";
        return false;
      }
      seen_pids.insert(pid.value());

      if (STLContains(seen_names, pid.name())) {
        OLA_WARN << "PID " << pid.name()
                 << " exists multiple times in the pid file";
        return false;
      }
      seen_names.insert(pid.name());

      if (limit_pid_values && pid.value() > MANUFACTURER_PID_MIN &&
          pid.value() < MANUFACTURER_PID_MAX) {
        OLA_WARN << "ESTA PID " << pid.name() << " (" << pid.value() << ")"
                 << " is outside acceptable range";
        return false;
      }
    }

    PidMap::iterator iter = STLLookupOrInsertNull(pid_map, pid.value());
    if (iter->second) {
      OLA_INFO << "Using " << OVERRIDE_FILE_NAME << " for " << pid.name()
               << "( " << strings::ToHex(pid.value()) << ")";
      continue;
    }

    const PidDescriptor *descriptor = PidToDescriptor(pid, validate);
    if (!descriptor) {
      return false;
    }
    iter->second = descriptor;
  }
  return true;
}

/*
 * Build a PidDescriptor from a Pid protobuf object
 */
PidDescriptor *PidStoreLoader::PidToDescriptor(const ola::rdm::pid::Pid &pid,
                                               bool validate) {
  // populate sub device validators
  PidDescriptor::sub_device_validator get_validator =
    PidDescriptor::ANY_SUB_DEVICE;
  if (pid.has_get_sub_device_range())
    get_validator = ConvertSubDeviceValidator(pid.get_sub_device_range());
  PidDescriptor::sub_device_validator set_validator =
    PidDescriptor::ANY_SUB_DEVICE;
  if (pid.has_set_sub_device_range())
    set_validator = ConvertSubDeviceValidator(pid.set_sub_device_range());

  // yuck, code smell. This should use protobuf reflections instead.
  const Descriptor *get_request = NULL;
  if (pid.has_get_request()) {
    get_request = FrameFormatToDescriptor(pid.get_request(), validate);
    if (!get_request)
      return NULL;
  }

  const Descriptor *get_response = NULL;
  if (pid.has_get_response()) {
    get_response = FrameFormatToDescriptor(pid.get_response(), validate);
    if (!get_response) {
      delete get_request;
      return NULL;
    }
  }

  const Descriptor *set_request = NULL;
  if (pid.has_set_request()) {
    set_request = FrameFormatToDescriptor(pid.set_request(), validate);
    if (!set_request) {
      delete get_request;
      delete get_response;
      return NULL;
    }
  }

  const Descriptor *set_response = NULL;
  if (pid.has_set_response()) {
    set_response = FrameFormatToDescriptor(pid.set_response(), validate);
    if (!set_response) {
      delete get_request;
      delete get_response;
      delete set_request;
      return NULL;
    }
  }

  PidDescriptor *descriptor = new PidDescriptor(
      pid.name(),
      pid.value(),
      get_request,
      get_response,
      set_request,
      set_response,
      get_validator,
      set_validator);
  return descriptor;
}

/*
 * Convert a protobuf frame format to a Descriptor object
 */
const Descriptor* PidStoreLoader::FrameFormatToDescriptor(
    const ola::rdm::pid::FrameFormat &format,
    bool validate) {
  bool ok = true;
  vector<const FieldDescriptor*> fields;

  for (int i = 0; i < format.field_size(); ++i) {
    const FieldDescriptor *field = FieldToFieldDescriptor(format.field(i));
    if (!field) {
      ok = false;
      break;
    }
    fields.push_back(field);
  }

  if (!ok) {
    vector<const FieldDescriptor*>::iterator iter = fields.begin();
    for (; iter != fields.end(); ++iter) {
      delete *iter;
    }
    return NULL;
  }

  // we don't give these descriptors names
  const Descriptor *descriptor = new Descriptor("", fields);

  if (validate) {
    if (!m_checker.CheckConsistency(descriptor)) {
      OLA_WARN << "Invalid frame format";
      delete descriptor;
      return NULL;
    }
  }
  return descriptor;
}

/*
 * Convert a protobuf field object to a FieldDescriptor.
 */
const FieldDescriptor *PidStoreLoader::FieldToFieldDescriptor(
    const ola::rdm::pid::Field &field) {
  const FieldDescriptor *descriptor = NULL;
  switch (field.type()) {
    case ola::rdm::pid::BOOL:
      descriptor = new ola::messaging::BoolFieldDescriptor(field.name());
      break;
    case ola::rdm::pid::UINT8:
      descriptor =
        IntegerFieldToFieldDescriptor<ola::messaging::UInt8FieldDescriptor>(
            field);
      break;
    case ola::rdm::pid::UINT16:
      descriptor =
        IntegerFieldToFieldDescriptor<ola::messaging::UInt16FieldDescriptor>(
            field);
      break;
    case ola::rdm::pid::UINT32:
      descriptor =
        IntegerFieldToFieldDescriptor<ola::messaging::UInt32FieldDescriptor>(
            field);
      break;
    case ola::rdm::pid::INT8:
      descriptor =
        IntegerFieldToFieldDescriptor<ola::messaging::Int8FieldDescriptor>(
            field);
      break;
    case ola::rdm::pid::INT16:
      descriptor =
        IntegerFieldToFieldDescriptor<ola::messaging::Int16FieldDescriptor>(
            field);
      break;
    case ola::rdm::pid::INT32:
      descriptor =
        IntegerFieldToFieldDescriptor<ola::messaging::Int32FieldDescriptor>(
            field);
      break;
    case ola::rdm::pid::STRING:
      descriptor = StringFieldToFieldDescriptor(field);
      break;
    case ola::rdm::pid::GROUP:
      descriptor = GroupFieldToFieldDescriptor(field);
      break;
    case ola::rdm::pid::IPV4:
      descriptor = new ola::messaging::IPV4FieldDescriptor(field.name());
      break;
    case ola::rdm::pid::MAC:
      descriptor = new ola::messaging::MACFieldDescriptor(field.name());
      break;
    case ola::rdm::pid::UID:
      descriptor = new ola::messaging::UIDFieldDescriptor(field.name());
      break;
    default:
      OLA_WARN << "Unknown field type: " << field.type();
  }
  return descriptor;
}

/*
 * Convert a integer protobuf field to a FieldDescriptor.
 */
template <typename descriptor_class>
const FieldDescriptor *PidStoreLoader::IntegerFieldToFieldDescriptor(
    const ola::rdm::pid::Field &field) {

  typename descriptor_class::IntervalVector intervals;
  typename descriptor_class::LabeledValues labels;

  for (int i = 0; i < field.range_size(); ++i) {
    const ola::rdm::pid::Range &range_value = field.range(i);
    typename descriptor_class::Interval interval(range_value.min(),
                                                 range_value.max());
    intervals.push_back(interval);
  }

  // if not intervals were specified, we automatically add all the labels
  bool intervals_empty = intervals.empty();

  for (int i = 0; i < field.label_size(); ++i) {
    const ola::rdm::pid::LabeledValue &labeled_value = field.label(i);
    labels[labeled_value.label()] = labeled_value.value();
    if (intervals_empty) {
      typename descriptor_class::Interval interval(labeled_value.value(),
                                                   labeled_value.value());
      intervals.push_back(interval);
    }
  }

  int8_t multiplier = 0;
  if (field.has_multiplier())
    multiplier = field.multiplier();

  return new descriptor_class(
      field.name(),
      intervals,
      labels,
      false,
      multiplier);
}

/*
 * Convert a string protobuf field to a FieldDescriptor.
 */
const FieldDescriptor *PidStoreLoader::StringFieldToFieldDescriptor(
    const ola::rdm::pid::Field &field) {
  uint8_t min = 0;

  if (field.has_min_size())
    min = field.min_size();

  if (!field.has_max_size()) {
    OLA_WARN << "String field failed to specify max size";
    return NULL;
  }
  return new ola::messaging::StringFieldDescriptor(
      field.name(),
      min,
      field.max_size());
}

/*
 * Convert a group protobuf field to a FieldDescriptor.
 */
const FieldDescriptor *PidStoreLoader::GroupFieldToFieldDescriptor(
    const ola::rdm::pid::Field &field) {
  vector<const class FieldDescriptor*> fields;
  bool ok = true;

  uint16_t min = 0;
  int16_t max = ola::messaging::FieldDescriptorGroup::UNLIMITED_BLOCKS;

  if (field.has_min_size())
    min = field.min_size();

  if (field.has_max_size())
    max = field.max_size();

  for (int i = 0; i < field.field_size(); ++i) {
    const FieldDescriptor *descriptor = FieldToFieldDescriptor(field.field(i));
    if (!descriptor) {
      ok = false;
      break;
    }
    fields.push_back(descriptor);
  }

  if (!ok) {
    vector<const class FieldDescriptor*>::iterator iter = fields.begin();
    for (; iter != fields.end(); ++iter) {
      delete *iter;
    }
    return NULL;
  }

  return new ola::messaging::FieldDescriptorGroup(
      field.name(),
      fields,
      min,
      max);
}

/*
 * Convert a protobuf sub device enum to a PidDescriptor one.
 */
PidDescriptor::sub_device_validator PidStoreLoader::ConvertSubDeviceValidator(
    const ola::rdm::pid::SubDeviceRange &sub_device_range) {
  switch (sub_device_range) {
    case ola::rdm::pid::ROOT_DEVICE:
      return PidDescriptor::ROOT_DEVICE;
    case ola::rdm::pid::ROOT_OR_ALL_SUBDEVICE:
      return PidDescriptor::ANY_SUB_DEVICE;
    case ola::rdm::pid::ROOT_OR_SUBDEVICE:
      return PidDescriptor::NON_BROADCAST_SUB_DEVICE;
    case ola::rdm::pid::ONLY_SUBDEVICES:
      return PidDescriptor::SPECIFIC_SUB_DEVICE;
    default:
      OLA_WARN << "Unknown sub device validator: " << sub_device_range <<
          ", defaulting to all";
      return PidDescriptor::ANY_SUB_DEVICE;
  }
}

void PidStoreLoader::FreeManufacturerMap(ManufacturerMap *data) {
  ManufacturerMap::iterator iter = data->begin();
  for (; iter != data->end(); ++iter) {
    STLDeleteValues(iter->second);
    delete iter->second;
  }
  data->clear();
}
}  // namespace rdm
}  // namespace ola
