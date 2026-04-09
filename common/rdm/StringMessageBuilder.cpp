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
 * StringMessageBuilder.cpp
 * Builds a Message object from a list of strings & a Descriptor.
 * Copyright (C) 2011 Simon Newton
 */


#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <ola/messaging/Descriptor.h>
#include <ola/messaging/Message.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/IPV6Address.h>
#include <ola/network/MACAddress.h>
#include <ola/rdm/StringMessageBuilder.h>
#include <ola/rdm/UID.h>
#include <memory>
#include <string>
#include <vector>

#include "common/rdm/GroupSizeCalculator.h"

namespace ola {
namespace rdm {

using ola::messaging::MessageFieldInterface;
using ola::rdm::UID;
using std::auto_ptr;
using std::string;
using std::vector;


StringMessageBuilder::StringMessageBuilder()
    : m_offset(0),
      m_input_size(0),
      m_group_instance_count(0),
      m_error(false) {
}


/**
 * @brief Clean up
 */
StringMessageBuilder::~StringMessageBuilder() {
  CleanUpVector();
}


/**
 * @brief Get the Message object that this Builder created
 *
 * This method is *not* re-entrant.
 * @param inputs the string inputs provided to build the Message
 * @param descriptor the descriptor to use to build the Message
 * @returns a Message object, or NULL if the inputs failed.
 */
const ola::messaging::Message *StringMessageBuilder::GetMessage(
    const vector<string> &inputs,
    const ola::messaging::Descriptor *descriptor) {
  InitVars(inputs);

  // first figure out if the number of inputs provided matches the number
  // expected by the descriptor. This accounts for repeating groups.
  GroupSizeCalculator calculator;
  GroupSizeCalculator::calculator_state state = calculator.CalculateGroupSize(
      inputs.size(),
      descriptor,
      &m_group_instance_count);

  switch (state) {
    case GroupSizeCalculator::INSUFFICIENT_TOKENS:
      SetError("Insufficient tokens");
      return NULL;
    case GroupSizeCalculator::EXTRA_TOKENS:
      SetError("Extra tokens");
      return NULL;
    case GroupSizeCalculator::MISMATCHED_TOKENS:
      SetError("Mismatched tokens");
      return NULL;
    case GroupSizeCalculator::MULTIPLE_VARIABLE_GROUPS:
      SetError("Multiple variable groups");
      return NULL;
    case GroupSizeCalculator::NESTED_VARIABLE_GROUPS:
      SetError("Nested variable groups");
      return NULL;
    case GroupSizeCalculator::SINGLE_VARIABLE_GROUP:
    case GroupSizeCalculator::NO_VARIABLE_GROUPS:
      break;
  }

  // now we now that this list of inputs can be parsed, and we know the number
  // of instances of a repeating group if there is one.
  descriptor->Accept(this);

  if (m_error) {
    OLA_WARN << "Error building message, field is: " << m_error_string;
    return NULL;
  }

  if (m_groups.size() != 1) {
    OLA_WARN << "Mismatched stack, size was " << m_groups.size();
    return NULL;
  }

  const ola::messaging::Message *message =  new ola::messaging::Message(
      m_groups.top());
  m_groups.top().clear();
  return message;
}


/**
 * Bool values can be true,false,0,1
 */
void StringMessageBuilder::Visit(
    const ola::messaging::BoolFieldDescriptor *descriptor) {
  if (StopParsing()) {
    return;
  }

  bool value = false;
  bool valid = false;
  string token = m_inputs[m_offset++];
  ola::StringTrim(&token);
  ola::ToLower(&token);

  if (token == "true") {
    valid = value = true;
  } else if (token == "false") {
    value = false;
    valid = true;
  }

  if (!valid) {
    uint8_t int_value;
    if (ola::StringToInt(token, &int_value)) {
      if (int_value == 1) {
        valid = value = true;
      } else if (int_value == 0) {
        valid = true;
        value = false;
      }
    }
  }

  if (!valid) {
    SetError(descriptor->Name());
    return;
  }

  m_groups.top().push_back(
      new ola::messaging::BoolMessageField(descriptor, value));
}


/**
 * IPV4 Addresses
 */
void StringMessageBuilder::Visit(
    const ola::messaging::IPV4FieldDescriptor *descriptor) {
  if (StopParsing()) {
    return;
  }

  string token = m_inputs[m_offset++];
  ola::network::IPV4Address ip_address;
  if (!ola::network::IPV4Address::FromString(token, &ip_address)) {
    SetError(descriptor->Name());
    return;
  }

  m_groups.top().push_back(
      new ola::messaging::IPV4MessageField(descriptor, ip_address));
}


/**
 * IPV6 Addresses
 */
void StringMessageBuilder::Visit(
    const ola::messaging::IPV6FieldDescriptor *descriptor) {
  if (StopParsing()) {
    return;
  }

  string token = m_inputs[m_offset++];
  ola::network::IPV6Address ipv6_address;
  if (!ola::network::IPV6Address::FromString(token, &ipv6_address)) {
    SetError(descriptor->Name());
    return;
  }

  m_groups.top().push_back(
      new ola::messaging::IPV6MessageField(descriptor, ipv6_address));
}


/**
 * MAC Addresses
 */
void StringMessageBuilder::Visit(
    const ola::messaging::MACFieldDescriptor *descriptor) {
  if (StopParsing()) {
    return;
  }

  string token = m_inputs[m_offset++];
  ola::network::MACAddress mac_address;
  if (!ola::network::MACAddress::FromString(token, &mac_address)) {
    SetError(descriptor->Name());
    return;
  }

  m_groups.top().push_back(
      new ola::messaging::MACMessageField(descriptor, mac_address));
}


/**
 * UIDs.
 */
void StringMessageBuilder::Visit(
    const ola::messaging::UIDFieldDescriptor *descriptor) {
  if (StopParsing()) {
    return;
  }

  string token = m_inputs[m_offset++];
  auto_ptr<UID> uid(UID::FromString(token));

  if (!uid.get()) {
    SetError(descriptor->Name());
    return;
  }

  m_groups.top().push_back(
      new ola::messaging::UIDMessageField(descriptor, *uid));
}


/**
 * Handle strings
 */
void StringMessageBuilder::Visit(
    const ola::messaging::StringFieldDescriptor *descriptor) {
  if (StopParsing()) {
    return;
  }

  const string &token = m_inputs[m_offset++];
  if (descriptor->MaxSize() != 0 &&
      token.size() > descriptor->MaxSize()) {
    SetError(descriptor->Name());
    return;
  }

  m_groups.top().push_back(
      new ola::messaging::StringMessageField(descriptor, token));
}


/**
 * uint8
 */
void StringMessageBuilder::Visit(
    const ola::messaging::UInt8FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


void StringMessageBuilder::Visit(
    const ola::messaging::UInt16FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


void StringMessageBuilder::Visit(
    const ola::messaging::UInt32FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


void StringMessageBuilder::Visit(
    const ola::messaging::UInt64FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


void StringMessageBuilder::Visit(
    const ola::messaging::Int8FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


void StringMessageBuilder::Visit(
    const ola::messaging::Int16FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


void StringMessageBuilder::Visit(
    const ola::messaging::Int32FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


void StringMessageBuilder::Visit(
    const ola::messaging::Int64FieldDescriptor *descriptor) {
  VisitInt(descriptor);
}


/**
 * Visit a group
 */
void StringMessageBuilder::Visit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {

  unsigned int iterations = descriptor->FixedSize() ? descriptor->MinBlocks() :
    m_group_instance_count;

  for (unsigned int i = 0; i < iterations; ++i) {
    vector<const MessageFieldInterface*> fields;
    m_groups.push(fields);

    for (unsigned int j = 0; j < descriptor->FieldCount(); ++j) {
      descriptor->GetField(j)->Accept(this);
    }

    const vector<const MessageFieldInterface*> &populated_fields =
        m_groups.top();
    const ola::messaging::MessageFieldInterface *message =
        new ola::messaging::GroupMessageField(descriptor, populated_fields);
    m_groups.pop();
    m_groups.top().push_back(message);
  }
}


/**
 * This is a no-op since we handle descending ourselves in Visit()
 */
void StringMessageBuilder::PostVisit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {
  (void) descriptor;
}


bool StringMessageBuilder::StopParsing() const {
  return m_error ||  m_offset >= m_input_size;
}


void StringMessageBuilder::SetError(const string &error) {
  m_error = true;
  m_error_string = error;
}


template<typename type>
void StringMessageBuilder::VisitInt(
    const ola::messaging::IntegerFieldDescriptor<type> *descriptor) {
  if (StopParsing()) {
    return;
  }

  type int_value;
  string input = m_inputs[m_offset++];
  if (descriptor->LookupLabel(input, &int_value) ||
      ola::PrefixedHexStringToInt(input, &int_value) ||
      ola::StringToInt(input, &int_value)) {
    if (descriptor->IsValid(int_value)) {
      m_groups.top().push_back(
          new ola::messaging::BasicMessageField<type>(descriptor, int_value));
    } else {
      SetError(descriptor->Name());
    }
  } else {
    SetError(descriptor->Name());
  }
}


void StringMessageBuilder::InitVars(const vector<string> &inputs) {
  CleanUpVector();
  // add the first fields vector to the stack
  vector<const MessageFieldInterface*> fields;
  m_groups.push(fields);

  m_inputs = inputs;
  m_input_size = inputs.size();
  m_error = false;
  m_offset = 0;
}


void StringMessageBuilder::CleanUpVector() {
  while (!m_groups.empty()) {
    const vector<const MessageFieldInterface*> &fields = m_groups.top();
    vector<const MessageFieldInterface*>::const_iterator iter = fields.begin();
    for (; iter != fields.end(); ++iter) {
      delete *iter;
    }
    m_groups.pop();
  }
}
}  // namespace rdm
}  // namespace ola
