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

#include <errno.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <fstream>
#include <set>
#include <string>
#include <vector>
#include "common/rdm/PidStoreLoader.h"
#include "common/rdm/Pids.pb.h"
#include "ola/Logging.h"
#include "ola/rdm/PidStore.h"
#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace rdm {

using ola::messaging::FieldDescriptor;
using std::map;
using std::set;
using std::vector;


/**
 * Load Pid information from a file.
 * @param file the path to the file to load
 * @param validate set to true if we should perform validation of the contents.
 * @returns A pointer to a new RootPidStore or NULL if loading failed.
 */
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


/**
 * Load Pid information from a string
 * @param file the path to the file to load
 * @param validate set to true if we should perform validation of the contents.
 * @returns A pointer to a new RootPidStore or NULL if loading failed.
 */
const RootPidStore *PidStoreLoader::LoadFromStream(std::istream *data,
                                                   bool validate) {
  ola::rdm::pid::PidStore pid_store_pb;
  google::protobuf::io::IstreamInputStream input_stream(data);
  bool ok = google::protobuf::TextFormat::Parse(&input_stream, &pid_store_pb);

  if (!ok)
    return NULL;

  return BuildStore(pid_store_pb, validate);
}


/**
 * Build the root store from a protocol buffer.
 */
const RootPidStore *PidStoreLoader::BuildStore(
    const ola::rdm::pid::PidStore &store_pb,
    bool validate) {
  RootPidStore::ManufacturerMap manufacturer_map;
  set<uint16_t> seen_values;
  set<string> seen_names;

  vector<const PidDescriptor*> pids;

  for (int i = 0; i < store_pb.pid_size(); ++i) {
    const ola::rdm::pid::Pid &pid = store_pb.pid(i);

    OLA_DEBUG << "Loading " << pid.name();
    if (validate) {
      set<uint16_t>::const_iterator value_iter = seen_values.find(pid.value());
      if (value_iter != seen_values.end()) {
        OLA_WARN << "Pid " << pid.value() << " exists multiple times in the "
            " pid file";
        return NULL;
      }
      seen_values.insert(pid.value());

      set<string>::const_iterator name_iter = seen_names.find(pid.name());
      if (name_iter != seen_names.end()) {
        OLA_WARN << "Pid " << pid.name() << " exists multiple times in the "
            " pid file";
        return NULL;
      }
      seen_names.insert(pid.name());

      if (pid.value() > 0x8000 and pid.value() < 0xffe0) {
        OLA_WARN << "ESTA Pid " << pid.name() << " (" << pid.value() << ")" <<
            " is outside acceptable range";
        return NULL;
      }
    }

    const PidDescriptor *descriptor = PidToDescriptor(pid, validate);
    if (!descriptor)
      return NULL;
    pids.push_back(descriptor);
  }

  OLA_DEBUG << "Load Complete";
  const PidStore *esta_store = new PidStore(pids);
  return new RootPidStore(esta_store, manufacturer_map);
}


/**
 * Build a PidDescriptor from a Pid protobuf object
 */
PidDescriptor *PidStoreLoader::PidToDescriptor(const ola::rdm::pid::Pid &pid,
                                             bool validate) {
  // populate sub device validators
  PidDescriptor::sub_device_valiator get_validator =
    PidDescriptor::ANY_SUB_DEVICE;
  if (pid.has_get_sub_device_range())
    get_validator = ConvertSubDeviceValidator(pid.get_sub_device_range());
  PidDescriptor::sub_device_valiator set_validator =
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


/**
 * Convert a protobuf frame format to a Descriptor object
 */
const Descriptor* PidStoreLoader::FrameFormatToDescriptor(
    const ola::rdm::pid::FrameFormat &format,
    bool validate) {
  bool ok = true;
  vector<const class FieldDescriptor*> fields;

  for (int i = 0; i < format.field_size(); ++i) {
    const FieldDescriptor *field = FieldToFieldDescriptor(format.field(i));
    if (!field) {
      ok = false;
      break;
    }
    fields.push_back(field);
  }

  if (!ok) {
    vector<const class FieldDescriptor*>::iterator iter = fields.begin();
    for (; iter != fields.end(); ++iter) {
      delete *iter;
    }
    return NULL;
  }

  // we don't give these requests names
  return new Descriptor("", fields);
  (void) validate;
}


/**
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
    default:
      OLA_WARN << "Unknown field type: " << field.type();
  }
  return descriptor;
}


/**
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

  int8_t multipler = 0;
  if (field.has_multiplier())
    multipler = field.multiplier();

  return new descriptor_class(
      field.name(),
      intervals,
      labels,
      false,
      multipler);
}

/**
 * Convert a string protobuf field to a FieldDescriptor.
 */
const FieldDescriptor *PidStoreLoader::StringFieldToFieldDescriptor(
    const ola::rdm::pid::Field &field) {
  uint8_t min = 0;

  if (field.has_min_size())
    min = field.min_size();

  if (!field.has_max_size()) {
    OLA_WARN << "String field failed to specific max size";
    return NULL;
  }
  return new ola::messaging::StringFieldDescriptor(
      field.name(),
      min,
      field.max_size());
}


/**
 * Convert a group protobuf field to a FieldDescriptor.
 */
const FieldDescriptor *PidStoreLoader::GroupFieldToFieldDescriptor(
    const ola::rdm::pid::Field &field) {
  vector<const class FieldDescriptor*> fields;
  bool ok = true;

  uint8_t min = 0;
  uint8_t max = 0;

  if (field.has_min_size())
    min = field.min_size();

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

  return new ola::messaging::GroupFieldDescriptor(
      field.name(),
      fields,
      min,
      max);
}


/**
 * Convert a protobuf sub device enum to a PidDescriptor one.
 */
PidDescriptor::sub_device_valiator PidStoreLoader::ConvertSubDeviceValidator(
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
}  // rdm
}  // ola
