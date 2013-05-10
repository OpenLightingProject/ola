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
 * Message.h
 * Holds the data for a message.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_MESSAGE_H_
#define INCLUDE_OLA_MESSAGING_MESSAGE_H_

#include <ola/messaging/Descriptor.h>
#include <ola/messaging/MessageVisitor.h>
#include <ola/network/IPV4Address.h>
#include <ola/rdm/UID.h>
#include <string>
#include <vector>

using std::string;
using std::vector;


namespace ola {
namespace messaging {


class MessageVisitor;

class Message {
  public:
    explicit Message(const vector<const class MessageFieldInterface*> &fields)
        : m_fields(fields) {
    }
    ~Message();

    void Accept(MessageVisitor &visitor) const;

    unsigned int FieldCount() const { return m_fields.size(); }

  private:
    vector<const class MessageFieldInterface*> m_fields;
};



/**
 * The Interface for a MessageField.
 */
class MessageFieldInterface {
  public:
    virtual ~MessageFieldInterface() {}

    // Call back into a MessageVisitor
    virtual void Accept(MessageVisitor &visitor) const = 0;
};



/**
 * A MessageField that represents a bool
 */
class BoolMessageField: public MessageFieldInterface {
  public:
    BoolMessageField(const BoolFieldDescriptor *descriptor,
                      bool value)
        : m_descriptor(descriptor),
          m_value(value) {
    }

    const BoolFieldDescriptor *GetDescriptor() const {
      return m_descriptor;
    }
    bool Value() const { return m_value; }

    void Accept(MessageVisitor &visitor) const {
      visitor.Visit(this);
    }

  private:
    const BoolFieldDescriptor *m_descriptor;
    bool m_value;
};


/**
 * A MessageField that represents a IPv4 Address
 */
class IPV4MessageField: public MessageFieldInterface {
  public:
    IPV4MessageField(const IPV4FieldDescriptor *descriptor,
                     const ola::network::IPV4Address &value)
        : m_descriptor(descriptor),
          m_value(value) {
    }

    IPV4MessageField(const IPV4FieldDescriptor *descriptor,
                     uint32_t value)
        : m_descriptor(descriptor),
          m_value(ola::network::IPV4Address(value)) {
    }

    const IPV4FieldDescriptor *GetDescriptor() const {
      return m_descriptor;
    }
    ola::network::IPV4Address Value() const { return m_value; }

    void Accept(MessageVisitor &visitor) const {
      visitor.Visit(this);
    }

  private:
    const IPV4FieldDescriptor *m_descriptor;
    ola::network::IPV4Address m_value;
};


/**
 * A MessageField that represents a UID.
 */
class UIDMessageField: public MessageFieldInterface {
  public:
    UIDMessageField(const UIDFieldDescriptor *descriptor,
                    const ola::rdm::UID &uid)
        : m_descriptor(descriptor),
          m_uid(uid) {
    }

    const UIDFieldDescriptor *GetDescriptor() const {
      return m_descriptor;
    }
    const ola::rdm::UID& Value() const { return m_uid; }

    void Accept(MessageVisitor &visitor) const {
      visitor.Visit(this);
    }

  private:
    const UIDFieldDescriptor *m_descriptor;
    ola::rdm::UID m_uid;
};


/**
 * A MessageField that represents a string
 */
class StringMessageField: public MessageFieldInterface {
  public:
    StringMessageField(const StringFieldDescriptor *descriptor,
                       const string &value)
        : m_descriptor(descriptor),
          m_value(value) {
    }

    const StringFieldDescriptor *GetDescriptor() const { return m_descriptor; }
    const string& Value() const { return m_value; }

    void Accept(MessageVisitor &visitor) const {
      visitor.Visit(this);
    }

  private:
    const StringFieldDescriptor *m_descriptor;
    const string m_value;
};


/**
 * A MessageField that represents an simple type
 */
template <typename type>
class BasicMessageField: public MessageFieldInterface {
  public:
    BasicMessageField(const IntegerFieldDescriptor<type> *descriptor,
                      type value)
        : m_descriptor(descriptor),
          m_value(value) {
    }

    const IntegerFieldDescriptor<type> *GetDescriptor() const {
      return m_descriptor;
    }
    type Value() const { return m_value; }

    void Accept(MessageVisitor &visitor) const {
      visitor.Visit(this);
    }

  private:
    const IntegerFieldDescriptor<type> *m_descriptor;
    type m_value;
};


typedef BasicMessageField<uint8_t> UInt8MessageField;
typedef BasicMessageField<uint16_t> UInt16MessageField;
typedef BasicMessageField<uint32_t> UInt32MessageField;
typedef BasicMessageField<int8_t> Int8MessageField;
typedef BasicMessageField<int16_t> Int16MessageField;
typedef BasicMessageField<int32_t> Int32MessageField;


/**
 * A MessageField that consists of a group of fields
 */
class GroupMessageField: public MessageFieldInterface {
  public:
    GroupMessageField(const FieldDescriptorGroup *descriptor,
                      const vector<const class MessageFieldInterface*> &fields)
      : m_descriptor(descriptor),
        m_fields(fields) {
    }
    ~GroupMessageField();

    const FieldDescriptorGroup *GetDescriptor() const { return m_descriptor; }

    unsigned int FieldCount() const { return m_fields.size(); }
    const class MessageFieldInterface *GetField(unsigned int index) const {
      if (index < m_fields.size())
        return m_fields[index];
      return NULL;
    }

    void Accept(MessageVisitor &visitor) const;

  private:
    const FieldDescriptorGroup *m_descriptor;
    vector<const class MessageFieldInterface*> m_fields;
};
}  // namespace messaging
}  // namespace ola

#endif  // INCLUDE_OLA_MESSAGING_MESSAGE_H_
