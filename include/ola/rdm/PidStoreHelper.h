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
 * PidStoreHelper.h
 * Provides helper methods for loading / accessing the pid store, and dealing
 * with pids.
 * Copyright (C) 2011 Simon Newton
 */

/**
 * @addtogroup rdm_pids
 * @{
 * @file PidStoreHelper.h
 * @brief Provides helper methods for loading / accessing the pid store, and
 * dealing with pids.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_PIDSTOREHELPER_H_
#define INCLUDE_OLA_RDM_PIDSTOREHELPER_H_

#include <stdint.h>
#include <ola/messaging/Descriptor.h>
#include <ola/messaging/SchemaPrinter.h>
#include <ola/rdm/MessageDeserializer.h>
#include <ola/rdm/MessageSerializer.h>
#include <ola/rdm/PidStore.h>
#include <ola/rdm/RDMMessagePrinters.h>
#include <ola/rdm/StringMessageBuilder.h>

#include <string>
#include <vector>


namespace ola {
namespace rdm {

class PidStoreHelper {
 public:
    explicit PidStoreHelper(const std::string &pid_location,
                            unsigned int initial_indent = 0);
    ~PidStoreHelper();

    bool Init();

    const PidDescriptor *GetDescriptor(
        const std::string &pid_name,
        uint16_t manufacturer_id) const;
    const PidDescriptor *GetDescriptor(
        uint16_t param_id,
        uint16_t manufacturer_id) const;

    const ola::messaging::Message *BuildMessage(
        const ola::messaging::Descriptor *descriptor,
        const std::vector<std::string> &inputs);

    const uint8_t *SerializeMessage(const ola::messaging::Message *message,
                                    unsigned int *data_length);

    const ola::messaging::Message *DeserializeMessage(
        const ola::messaging::Descriptor *descriptor,
        const uint8_t *data,
        unsigned int data_length);

    const std::string MessageToString(const ola::messaging::Message *message);

    const std::string PrettyPrintMessage(
        uint16_t manufacturer_id,
        bool is_set,
        uint16_t pid,
        const ola::messaging::Message *message);

    const std::string SchemaAsString(
        const ola::messaging::Descriptor *descriptor);

    void SupportedPids(uint16_t manufacturer_id,
                       std::vector<std::string> *pid_names) const;

    void SupportedPids(
        uint16_t manufacturer_id,
        std::vector<const PidDescriptor*> *descriptors) const;

 private:
    const std::string m_pid_location;
    const RootPidStore *m_root_store;
    StringMessageBuilder m_string_builder;
    MessageSerializer m_serializer;
    MessageDeserializer m_deserializer;
    RDMMessagePrinter m_message_printer;
    ola::messaging::SchemaPrinter m_schema_printer;

    static bool CompPidsForSort(const PidDescriptor* a, const PidDescriptor* b);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_PIDSTOREHELPER_H_
