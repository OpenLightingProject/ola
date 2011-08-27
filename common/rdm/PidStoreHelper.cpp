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
 * PidStoreHelper.cpp
 * Provides helper methods for loading / accessing the pid store, and dealing
 * with pids.
 * Copyright (C) 2011 Simon Newton
 */

#include <string>
#include <vector>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/rdm/PidStoreHelper.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/rdm/RDMMessagePrinters.h"

namespace ola {
namespace rdm {

using std::vector;

/**
 * Set up a new PidStoreHelper object
 */
PidStoreHelper::PidStoreHelper(const string &pid_file)
    : m_pid_file(pid_file),
      m_root_store(NULL) {
}


/**
 * Clean up
 */
PidStoreHelper::~PidStoreHelper() {
  if (m_root_store)
    delete m_root_store;
}


/**
 * Init the PidStoreHelper, this loads the pid store
 */
bool PidStoreHelper::Init() {
  if (m_root_store) {
    OLA_WARN << "Root Pid Store already loaded: " << m_pid_file;
    return false;
  }

  m_root_store = ola::rdm::RootPidStore::LoadFromFile(m_pid_file);
  return m_root_store;
}


/**
 * Lookup a PidDescriptor by name.
 * @param manufacturer_id the ESTA id of the manufacturer_id
 * @param pid_name the name of the pid
 * @return a PidDescriptor or NULL if the pid wasn't found.
 */
const ola::rdm::PidDescriptor *PidStoreHelper::GetDescriptor(
    const string &pid_name,
    uint16_t manufacturer_id) const {
  if (!m_root_store)
    return NULL;
  return m_root_store->GetDescriptor(pid_name, manufacturer_id);
}


/**
 * Lookup a PidDescriptor by pid value.
 * @param manufacturer_id the ESTA id of the manufacturer_id
 * @param pid_value the pid to lookup
 * @return a PidDescriptor or NULL if the pid wasn't found.
 */
const ola::rdm::PidDescriptor *PidStoreHelper::GetDescriptor(
    uint16_t pid_value,
    uint16_t manufacturer_id) const {
  if (!m_root_store)
    return NULL;
  return m_root_store->GetDescriptor(pid_value, manufacturer_id);
}


/**
 * Build a Message object from a series of input strings
 */
const ola::messaging::Message *PidStoreHelper::BuildMessage(
    const ola::messaging::Descriptor *descriptor,
    const vector<string> &inputs) {

  const ola::messaging::Message *message = m_string_builder.GetMessage(
      inputs,
      descriptor);
  if (!message)
    OLA_WARN << "Error building message: " << m_string_builder.GetError();
  return message;
}


/**
 * Serialize a message to binary format
 */
const uint8_t *PidStoreHelper::SerializeMessage(
    const ola::messaging::Message *message,
    unsigned int *data_length) {
  return m_serializer.SerializeMessage(message, data_length);
}



/**
 * DeSerialize a message
 */
const ola::messaging::Message *PidStoreHelper::DeserializeMessage(
    const ola::messaging::Descriptor *descriptor,
    const uint8_t *data,
    unsigned int data_length) {
  return m_deserializer.InflateMessage(descriptor, data, data_length);
}


/**
 * Convert a message to a string
 * @param message the Message object to print
 * @returns a formatted string representation of the message.
 */
const string PidStoreHelper::MessageToString(
    const ola::messaging::Message *message) {
  return m_message_printer.AsString(message);
}


/**
 * Pretty print a RDM message based on the PID, if we can't find a custom
 * MessagePrinter we default to the GenericMessagePrinter.
 * @param pid the pid value
 * @param message the Message object to print
 * @returns a formatted string representation of the message.
 */
const string PidStoreHelper::PrettyPrintMessage(
    uint16_t manufacturer_id,
    bool is_set,
    uint16_t pid,
    const ola::messaging::Message *message) {

  // switch based on command class and pid
  if (is_set) {
    { }
  } else {
    switch (pid) {
      case PID_PROXIED_DEVICES: {
        ProxiedDevicesPrinter printer;
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
    }
  }

  return m_message_printer.AsString(message);
}


/**
 * Return a string describing the schema for a descriptor
 */
const string PidStoreHelper::SchemaAsString(
    const ola::messaging::Descriptor *descriptor) {
  m_schema_printer.Reset();
  descriptor->Accept(m_schema_printer);
  return m_schema_printer.AsString();
}


/**
 * Return the list of pids supported including manufacturer pids.
 */
void PidStoreHelper::SupportedPids(uint16_t manufacturer_id,
                                   vector<string> *pid_names) const {
  if (!m_root_store)
    return;

  vector<const ola::rdm::PidDescriptor*> descriptors;
  const ola::rdm::PidStore *store = m_root_store->EstaStore();
  if (store)
    store->AllPids(&descriptors);

  store = m_root_store->ManufacturerStore(manufacturer_id);
  if (store)
    store->AllPids(&descriptors);

  vector<const ola::rdm::PidDescriptor*>::const_iterator iter;
  for (iter = descriptors.begin(); iter != descriptors.end(); ++iter) {
    string name = (*iter)->Name();
    ola::ToLower(&name);
    pid_names->push_back(name);
  }
}
}  // rdm
}  // ola
