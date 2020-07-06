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
 * PidStoreHelper.cpp
 * Provides helper methods for loading / accessing the pid store, and dealing
 * with pids.
 * Copyright (C) 2011 Simon Newton
 */

#include <algorithm>
#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/rdm/PidStoreHelper.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMMessagePrinters.h"

namespace ola {
namespace rdm {

using std::string;
using std::vector;
using std::sort;

/**
 * @brief Set up a new PidStoreHelper object
 */
PidStoreHelper::PidStoreHelper(const string &pid_location,
                               unsigned int initial_indent)
    : m_pid_location(pid_location.empty() ? RootPidStore::DataLocation() :
                     pid_location),
      m_root_store(NULL),
      m_message_printer(initial_indent) {
}


/**
 * @brief Clean up
 */
PidStoreHelper::~PidStoreHelper() {
  if (m_root_store) {
    delete m_root_store;
  }
}


/**
 * @brief Init the PidStoreHelper, this loads the PID store
 */
bool PidStoreHelper::Init() {
  if (m_root_store) {
    OLA_WARN << "Root PID Store already loaded from: " << m_pid_location;
    return false;
  }

  m_root_store = ola::rdm::RootPidStore::LoadFromDirectory(m_pid_location);
  return m_root_store;
}


/**
 * @brief Lookup a PidDescriptor by name.
 * @param manufacturer_id the ESTA id of the manufacturer_id
 * @param pid_name the name of the pid
 * @return a PidDescriptor or NULL if the pid wasn't found.
 */
const ola::rdm::PidDescriptor *PidStoreHelper::GetDescriptor(
    const string &pid_name,
    uint16_t manufacturer_id) const {
  if (!m_root_store) {
    return NULL;
  }
  return m_root_store->GetDescriptor(pid_name, manufacturer_id);
}


/**
 * @brief Lookup a PidDescriptor by PID value.
 * @param manufacturer_id the ESTA id of the manufacturer_id
 * @param pid_value the pid to lookup
 * @return a PidDescriptor or NULL if the pid wasn't found.
 */
const ola::rdm::PidDescriptor *PidStoreHelper::GetDescriptor(
    uint16_t pid_value,
    uint16_t manufacturer_id) const {
  if (!m_root_store) {
    return NULL;
  }
  return m_root_store->GetDescriptor(pid_value, manufacturer_id);
}


/**
 * @brief Build a Message object from a series of input strings
 */
const ola::messaging::Message *PidStoreHelper::BuildMessage(
    const ola::messaging::Descriptor *descriptor,
    const vector<string> &inputs) {

  const ola::messaging::Message *message = m_string_builder.GetMessage(
      inputs,
      descriptor);
  if (!message) {
    OLA_WARN << "Error building message: " << m_string_builder.GetError();
  }
  return message;
}


/**
 * @brief Serialize a message to binary format
 */
const uint8_t *PidStoreHelper::SerializeMessage(
    const ola::messaging::Message *message,
    unsigned int *data_length) {
  return m_serializer.SerializeMessage(message, data_length);
}



/**
 * @brief DeSerialize a message
 */
const ola::messaging::Message *PidStoreHelper::DeserializeMessage(
    const ola::messaging::Descriptor *descriptor,
    const uint8_t *data,
    unsigned int data_length) {
  return m_deserializer.InflateMessage(descriptor, data, data_length);
}


/**
 * @brief Convert a message to a string
 * @param message the Message object to print
 * @returns a formatted string representation of the message.
 */
const string PidStoreHelper::MessageToString(
    const ola::messaging::Message *message) {
  return m_message_printer.AsString(message);
}


/**
 * @brief Pretty print a RDM message based on the PID.
 *
 * If we can't find a custom MessagePrinter we default to the
 * GenericMessagePrinter.
 * @param manufacturer_id the manufacturer ID
 * @param is_set true if the message is a set command, false otherwise
 * @param pid the pid value
 * @param message the Message object to print
 * @returns a formatted string representation of the message.
 */
const string PidStoreHelper::PrettyPrintMessage(
    uint16_t manufacturer_id,
    bool is_set,
    uint16_t pid,
    const ola::messaging::Message *message) {

  // switch based on command class and PID
  if (is_set) {
    { }
  } else {
    switch (pid) {
      case PID_PROXIED_DEVICES: {
        ProxiedDevicesPrinter printer;
        return printer.AsString(message);
      }
      case PID_STATUS_MESSAGES: {
        StatusMessagePrinter printer;
        return printer.AsString(message);
      }
      case PID_SUPPORTED_PARAMETERS: {
        SupportedParamsPrinter printer(manufacturer_id, m_root_store);
        return printer.AsString(message);
      }
      case PID_DEVICE_INFO: {
        DeviceInfoPrinter printer;
        return printer.AsString(message);
      }
      case PID_PRODUCT_DETAIL_ID_LIST: {
        ProductIdPrinter printer;
        return printer.AsString(message);
      }
      case PID_DEVICE_MODEL_DESCRIPTION:
      case PID_MANUFACTURER_LABEL:
      case PID_DEVICE_LABEL:
      case PID_SOFTWARE_VERSION_LABEL:
      case PID_BOOT_SOFTWARE_VERSION_LABEL: {
        LabelPrinter printer;
        return printer.AsString(message);
      }
      case PID_LANGUAGE_CAPABILITIES: {
        LanguageCapabilityPrinter printer;
        return printer.AsString(message);
      }
      case PID_REAL_TIME_CLOCK: {
        ClockPrinter printer;
        return printer.AsString(message);
      }
      case PID_SENSOR_DEFINITION: {
        SensorDefinitionPrinter printer;
        return printer.AsString(message);
      }
      case PID_SLOT_INFO: {
        SlotInfoPrinter printer;
        return printer.AsString(message);
      }
    }
  }

  return m_message_printer.AsString(message);
}


/**
 * @brief Return a string describing the schema for a descriptor
 */
const string PidStoreHelper::SchemaAsString(
    const ola::messaging::Descriptor *descriptor) {
  m_schema_printer.Reset();
  descriptor->Accept(&m_schema_printer);
  return m_schema_printer.AsString();
}


/**
 * @brief Return the list of PIDs supported including manufacturer PIDs.
 */
void PidStoreHelper::SupportedPids(uint16_t manufacturer_id,
                                   vector<string> *pid_names) const {
  if (!m_root_store) {
    return;
  }

  vector<const ola::rdm::PidDescriptor*> descriptors;
  const ola::rdm::PidStore *store = m_root_store->EstaStore();
  if (store) {
    store->AllPids(&descriptors);
  }

  store = m_root_store->ManufacturerStore(manufacturer_id);
  if (store) {
    store->AllPids(&descriptors);
  }

  vector<const ola::rdm::PidDescriptor*>::const_iterator iter;
  for (iter = descriptors.begin(); iter != descriptors.end(); ++iter) {
    string name = (*iter)->Name();
    ola::ToLower(&name);
    pid_names->push_back(name);
  }
}


/**
 * @brief Return the list of PidDescriptors supported.
 *
 * The caller does not own the pointers, they are valid for the lifetime of the
 * PidStoreHelper.
 */
void PidStoreHelper::SupportedPids(
    uint16_t manufacturer_id,
    vector<const PidDescriptor*> *descriptors) const {
  if (!m_root_store) {
    return;
  }

  const ola::rdm::PidStore *store = m_root_store->EstaStore();
  if (store) {
    store->AllPids(descriptors);
  }

  store = m_root_store->ManufacturerStore(manufacturer_id);
  if (store) {
    store->AllPids(descriptors);
  }
}
}  // namespace rdm
}  // namespace ola
