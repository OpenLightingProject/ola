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
 * PidStoreHelper.h
 * This class does the heavy lifting for loading the PidStore from a protobuf.
 * It's separate so that PidStore.h doesn't have to include the Pids.pb.h
 * header.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef COMMON_RDM_PIDSTORELOADER_H_
#define COMMON_RDM_PIDSTORELOADER_H_

#include <ola/messaging/Descriptor.h>
#include <ola/rdm/PidStore.h>
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
    ~PidStoreLoader() {}

    // Load information into this store
    const RootPidStore *LoadFromFile(const std::string &file,
                                     bool validate = true);

    // Load information into this store
    const RootPidStore *LoadFromDirectory(const std::string &directory,
                                          bool validate = true);

    // Load information into this store
    const RootPidStore *LoadFromStream(std::istream *data,
                                       bool validate = true);

  private:
    PidStoreLoader(const PidStoreLoader&);
    PidStoreLoader& operator=(const PidStoreLoader&);
    DescriptorConsistencyChecker m_checker;

    const RootPidStore *BuildStore(const ola::rdm::pid::PidStore &store_pb,
                                   bool validate);
    template <typename pb_object>
    bool GetPidList(vector<const PidDescriptor*> *pids,
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
    PidDescriptor::sub_device_valiator ConvertSubDeviceValidator(
        const ola::rdm::pid::SubDeviceRange &sub_device_range);
    void CleanStore();
};
}  // rdm
}  // ola
#endif  // COMMON_RDM_PIDSTORELOADER_H_
