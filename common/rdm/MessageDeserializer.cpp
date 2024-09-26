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
 * MessageDeserializer.cpp
 * Inflate a Message object from raw data.
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/StringUtils.h>
#include <ola/messaging/Message.h>
#include <ola/network/NetworkUtils.h>
#include <ola/rdm/MessageDeserializer.h>
#include <ola/rdm/UID.h>
#include <string.h>
#include <string>
#include <vector>
#include "common/rdm/VariableFieldSizeCalculator.h"

namespace ola {
namespace rdm {

using ola::messaging::MessageFieldInterface;
using std::string;
using std::vector;

MessageDeserializer::MessageDeserializer()
    : m_data(NULL),
      m_length(0),
      m_offset(0),
      m_variable_field_size(0),
      m_insufficient_data(false) {
}


MessageDeserializer::~MessageDeserializer() {
  CleanUpVector();
}


/**
 * @brief Deserialize a memory location and return a message
 */
const ola::messaging::Message *MessageDeserializer::InflateMessage(
    const ola::messaging::Descriptor *descriptor,
    const uint8_t *data,
    unsigned int length) {

  if (!data && length) {
    return NULL;
  }

  m_data = data;
  m_length = length;
  m_offset = 0;
  m_insufficient_data = false;

  CleanUpVector();

  VariableFieldSizeCalculator calculator;
  VariableFieldSizeCalculator::calculator_state state =
    calculator.CalculateFieldSize(
        length,
        descriptor,
        &m_variable_field_size);

  switch (state) {
      case VariableFieldSizeCalculator::TOO_SMALL:
      case VariableFieldSizeCalculator::TOO_LARGE:
        return NULL;
      case VariableFieldSizeCalculator::FIXED_SIZE:
      case VariableFieldSizeCalculator::VARIABLE_STRING:
      case VariableFieldSizeCalculator::VARIABLE_GROUP:
        break;
      case VariableFieldSizeCalculator::MULTIPLE_VARIABLE_FIELDS:
      case VariableFieldSizeCalculator::NESTED_VARIABLE_GROUPS:
      case VariableFieldSizeCalculator::MISMATCHED_SIZE:
        return NULL;
  }

  message_vector root_messages;
  m_message_stack.push(root_messages);

  descriptor->Accept(this);

  // this should never trigger because we check the length in the
  // VariableFieldSizeCalculator
  if (m_insufficient_data) {
    return NULL;
  }

  const ola::messaging::Message *message =  new ola::messaging::Message(
      m_message_stack.top());
  m_message_stack.top().clear();
  return message;
}


void MessageDeserializer::Visit(
    const ola::messaging::BoolFieldDescriptor *descriptor) {
  if (!CheckForData(descriptor->MaxSize())) {
    return;
  }

  m_message_stack.top().push_back(
      new ola::messaging::BoolMessageField(descriptor, m_data[m_offset++]));
}


void MessageDeserializer::Visit(
    const ola::messaging::IPV4FieldDescriptor *descriptor) {
  if (!CheckForData(descriptor->MaxSize())) {
    return;
  }

  uint32_t data;
  memcpy(&data, m_data + m_offset, sizeof(data));
  m_offset += sizeof(data);
  m_message_stack.top().push_back(
      new ola::messaging::IPV4MessageField(
          descriptor,
          ola::network::IPV4Address(data)));
}


void MessageDeserializer::Visit(
    const ola::messaging::IPV6FieldDescriptor *descriptor) {
  if (!CheckForData(descriptor->MaxSize())) {
    return;
  }

  ola::network::IPV6Address ipv6_address(m_data + m_offset);
  m_offset += descriptor->MaxSize();
  m_message_stack.top().push_back(
      new ola::messaging::IPV6MessageField(descriptor, ipv6_address));
}


void MessageDeserializer::Visit(
    const ola::messaging::MACFieldDescriptor *descriptor) {
  if (!CheckForData(descriptor->MaxSize())) {
    return;
  }

  ola::network::MACAddress mac_address(m_data + m_offset);
  m_offset += descriptor->MaxSize();
  m_message_stack.top().push_back(
      new ola::messaging::MACMessageField(descriptor, mac_address));
}


void MessageDeserializer::Visit(
    const ola::messaging::UIDFieldDescriptor *descriptor) {
  if (!CheckForData(descriptor->MaxSize())) {
    return;
  }

  ola::rdm::UID uid(m_data + m_offset);
  m_offset += descriptor->MaxSize();
  m_message_stack.top().push_back(
      new ola::messaging::UIDMessageField(descriptor, uid));
}


void MessageDeserializer::Visit(
    const ola::messaging::StringFieldDescriptor *descriptor) {
  unsigned int string_size;

  if (descriptor->FixedSize()) {
    string_size = descriptor->MaxSize();
  } else {
    // variable sized string, the length is in m_variable_field_size
    string_size = m_variable_field_size;
  }

  if (!CheckForData(string_size)) {
    return;
  }

  string value(reinterpret_cast<const char *>(m_data + m_offset), string_size);
  ShortenString(&value);
  m_offset += string_size;
  m_message_stack.top().push_back(
      new ola::messaging::StringMessageField(descriptor, value));
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<uint8_t> *descriptor) {
  IntVisit(descriptor);
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<uint16_t> *descriptor) {
  IntVisit(descriptor);
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<uint32_t> *descriptor) {
  IntVisit(descriptor);
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<uint64_t> *descriptor) {
  IntVisit(descriptor);
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<int8_t> *descriptor) {
  IntVisit(descriptor);
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<int16_t> *descriptor) {
  IntVisit(descriptor);
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<int32_t> *descriptor) {
  IntVisit(descriptor);
}


void MessageDeserializer::Visit(
    const ola::messaging::IntegerFieldDescriptor<int64_t> *descriptor) {
  IntVisit(descriptor);
}


/**
 * @brief Visit a group field
 */
void MessageDeserializer::Visit(
    const ola::messaging::FieldDescriptorGroup *descriptor) {

  unsigned int iterations = descriptor->FixedSize() ? descriptor->MinBlocks() :
      m_variable_field_size;

  for (unsigned int i = 0; i < iterations; ++i) {
    vector<const MessageFieldInterface*> fields;
    m_message_stack.push(fields);

    for (unsigned int j = 0; j < descriptor->FieldCount(); ++j) {
      descriptor->GetField(j)->Accept(this);
    }

    const vector<const MessageFieldInterface*> &populated_fields =
        m_message_stack.top();
    const ola::messaging::MessageFieldInterface *message =
        new ola::messaging::GroupMessageField(descriptor, populated_fields);
    m_message_stack.pop();
    m_message_stack.top().push_back(message);
  }
}


/**
 * @brief Check that there is at least required_size bytes of data left.
 */
bool MessageDeserializer::CheckForData(unsigned int required_size) {
  if (required_size <= m_length - m_offset) {
    return true;
  }
  m_insufficient_data = true;
  return false;
}



/**
 * @brief Remove any old messages from the stack.
 */
void MessageDeserializer::CleanUpVector() {
  while (!m_message_stack.empty()) {
    const message_vector &fields = m_message_stack.top();
    message_vector::const_iterator iter = fields.begin();
    for (; iter != fields.end(); ++iter) {
      delete *iter;
    }
    m_message_stack.pop();
  }
}


/**
 * @brief Deserialize an integer value, converting from little endian if needed
 */
template <typename int_type>
void MessageDeserializer::IntVisit(
    const ola::messaging::IntegerFieldDescriptor<int_type> *descriptor) {
  if (!CheckForData(sizeof(int_type))) {
    return;
  }

  int_type value;

  memcpy(reinterpret_cast<uint8_t*>(&value),
         m_data + m_offset,
         sizeof(int_type));
  m_offset += sizeof(int_type);

  if (descriptor->IsLittleEndian()) {
    value = ola::network::LittleEndianToHost(value);
  } else {
    value = ola::network::NetworkToHost(value);
  }

  m_message_stack.top().push_back(
      new ola::messaging::BasicMessageField<int_type>(descriptor, value));
}
}  // namespace rdm
}  // namespace ola
