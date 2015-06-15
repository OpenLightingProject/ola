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
 * PidStoreLoader.h
 * This class does the heavy lifting for loading the PidStore from a protobuf.
 * It's separate so that PidStore.h doesn't have to include the Pids.pb.h
 * header.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef COMMON_RDM_PIDSTORELOADER_H_
#define COMMON_RDM_PIDSTORELOADER_H_

#include <ola/messaging/Descriptor.h>
#include <ola/rdm/PidStore.h>
#include <map>
#include <istream>
#include <string>
#include <vector>
#include "common/rdm/DescriptorConsistencyChecker.h"
#include "common/rdm/Pids.pb.h"

namespace ola {
namespace rdm {

/**
 * The PidStore Loader
 */
class PidStoreLoader {
 public:
  PidStoreLoader() {}

  /**
   * @brief Load PID information from a file.
   * @param file the path to the file to load
   * @param validate set to true if we should perform validation of the
   *   contents.
   * @returns A pointer to a new RootPidStore or NULL if loading failed.
   */
  const RootPidStore *LoadFromFile(const std::string &file,
                                   bool validate = true);

  /**
   * @brief Load PID information from a directory, including overrides.
   * @param directory the directory to load files from.
   * @param validate set to true if we should perform validation of the
   *   contents.
   * @returns A pointer to a new RootPidStore or NULL if loading failed.
   *
   * This is an all-or-nothing load. Any error with cause us to abort the load.
   */
  const RootPidStore *LoadFromDirectory(const std::string &directory,
                                        bool validate = true);

  /**
   * @brief Load Pid information from a stream
   * @param data the input stream.
   * @param validate set to true if we should perform validation of the
   *   contents.
   * @returns A pointer to a new RootPidStore or NULL if loading failed.
   */
  const RootPidStore *LoadFromStream(std::istream *data,
                                     bool validate = true);

 private:
  typedef std::map<uint16_t, const PidDescriptor*> PidMap;
  typedef std::map<uint16_t, PidMap*> ManufacturerMap;

  DescriptorConsistencyChecker m_checker;

  bool ReadFile(const std::string &file_path,
                ola::rdm::pid::PidStore *proto);

  const RootPidStore *BuildStore(const ola::rdm::pid::PidStore &store_pb,
                                 const ola::rdm::pid::PidStore &override_pb,
                                 bool validate);

  bool LoadFromProto(ManufacturerMap *pid_data,
                     const ola::rdm::pid::PidStore &proto,
                     bool validate);

  template <typename pb_object>
  bool GetPidList(PidMap *pid_map,
                  const pb_object &store,
                  bool validate,
                  bool limit_pid_values);

  PidDescriptor *PidToDescriptor(const ola::rdm::pid::Pid &pid,
                                 bool validate);
  const ola::messaging::Descriptor* FrameFormatToDescriptor(
      const ola::rdm::pid::FrameFormat &format,
      bool validate);
  const ola::messaging::FieldDescriptor *FieldToFieldDescriptor(
      const ola::rdm::pid::Field &field);

  template <typename descriptor_class>
  const ola::messaging::FieldDescriptor *IntegerFieldToFieldDescriptor(
      const ola::rdm::pid::Field &field);

  const ola::messaging::FieldDescriptor *StringFieldToFieldDescriptor(
      const ola::rdm::pid::Field &field);
  const ola::messaging::FieldDescriptor *GroupFieldToFieldDescriptor(
      const ola::rdm::pid::Field &field);
  PidDescriptor::sub_device_validator ConvertSubDeviceValidator(
      const ola::rdm::pid::SubDeviceRange &sub_device_range);

  void FreeManufacturerMap(ManufacturerMap *data);

  static const char OVERRIDE_FILE_NAME[];
  static const uint16_t ESTA_MANUFACTURER_ID;
  static const uint16_t MANUFACTURER_PID_MIN;
  static const uint16_t MANUFACTURER_PID_MAX;

  DISALLOW_COPY_AND_ASSIGN(PidStoreLoader);
};
}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_PIDSTORELOADER_H_
