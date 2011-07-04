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
 * Descriptor.h
 * Holds the metadata (schema) for a Message.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_DESCRIPTOR_H_
#define INCLUDE_OLA_MESSAGING_DESCRIPTOR_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <string>
#include <vector>

using std::string;
using std::vector;


namespace ola {
namespace messaging {


class FieldDescriptorVisitor;

class Descriptor {
  public:
    Descriptor(const string &name,
               const vector<class FieldDescriptor*> &fields)
        : m_name(name),
          m_fields(fields) {
    }

    const string &Name() const { return m_name; }
    unsigned int FieldCount() const { return m_fields.size(); }
    const class FieldDescriptor *GetField(unsigned int index) const {
      if (index < m_fields.size())
        return m_fields[index];
      return NULL;
    }

    void Accept(FieldDescriptorVisitor &visitor) const;

    /*
    Message *NewMessage() {
      return new Message(this);
    }
    */

  private:
    string m_name;
    vector<class FieldDescriptor *> m_fields;
};



/**
 * Describes a field, which may be a group of sub fields.
 */
class FieldDescriptorInterface {
  public:
    virtual ~FieldDescriptorInterface() {}

    // Returns the name of this field
    virtual const string& Name() const = 0;

    // Call back into a FieldDescriptorVisitor
    virtual void Accept(FieldDescriptorVisitor &visitor) const = 0;

    // Returns true if the size of this field is constant
    virtual bool FixedSize() const = 0;

    // Returns the largest size for this field
    virtual unsigned int Size() const = 0;
};


/**
 * Describes a field, which may be a group of sub fields
 */
class FieldDescriptor: public FieldDescriptorInterface {
  public:
    explicit FieldDescriptor(const string &name)
        : m_name(name) {
    }
    virtual ~FieldDescriptor() {}

    // Returns the name of this field
    const string& Name() const { return m_name; }

  private:
    string m_name;
};


/**
 * A FieldDescriptor that represents a bool
 */
class BoolFieldDescriptor: public FieldDescriptor {
  public:
    explicit BoolFieldDescriptor(const string &name)
        : FieldDescriptor(name) {
    }

    bool FixedSize() const { return true; }
    unsigned int Size() const { return 1; }

    void Accept(FieldDescriptorVisitor &visitor) const {
      visitor.Visit(this);
    }
};


/**
 * A FieldDescriptor that represents a string
 */
class StringFieldDescriptor: public FieldDescriptor {
  public:
    StringFieldDescriptor(const string &name,
                          uint8_t min_size,
                          uint8_t max_size)
        : FieldDescriptor(name),
          m_min_size(min_size),
          m_max_size(max_size) {
    }

    bool FixedSize() const { return m_min_size == m_max_size; }
    unsigned int Size() const { return m_max_size; }
    unsigned int MinSize() const { return m_min_size; }
    unsigned int MaxSize() const { return m_max_size; }

    void Accept(FieldDescriptorVisitor &visitor) const {
      visitor.Visit(this);
    }

  private:
    uint8_t m_min_size, m_max_size;
};


/**
 * A FieldDescriptor that represents an integer type
 */
template <typename type>
class IntegerFieldDescriptor: public FieldDescriptor {
  public:
    IntegerFieldDescriptor(const string &name,
                           bool little_endian = false,
                           int8_t multiplier = 0)
        : FieldDescriptor(name),
          m_little_endian(little_endian),
          m_multipler(multiplier) {
    }

    bool FixedSize() const { return true; }
    unsigned int Size() const { return sizeof(type); }
    int8_t Multiplier() const { return m_multipler; }
    bool IsLittleEndian() const { return m_little_endian; }

    void Accept(FieldDescriptorVisitor &visitor) const {
      visitor.Visit(this);
    }

    // list of Ranges
    // list of LabeledValues

  private:
    bool m_little_endian;
    int8_t m_multipler;
};


typedef IntegerFieldDescriptor<uint8_t> UInt8FieldDescriptor;
typedef IntegerFieldDescriptor<uint16_t> UInt16FieldDescriptor;
typedef IntegerFieldDescriptor<uint32_t> UInt32FieldDescriptor;
typedef IntegerFieldDescriptor<int8_t> Int8FieldDescriptor;
typedef IntegerFieldDescriptor<int16_t> Int16FieldDescriptor;
typedef IntegerFieldDescriptor<int32_t> Int32FieldDescriptor;


/**
 * A FieldDescriptor that consists of a group of fields
 */
class GroupFieldDescriptor: public FieldDescriptor {
  public:
    GroupFieldDescriptor(const string &name,
                         const vector<class FieldDescriptor*> &fields,
                         uint8_t min_size,
                         uint8_t max_size)
      : FieldDescriptor(name),
        m_fields(fields),
        m_min_size(min_size),
        m_max_size(max_size) {
    }

    bool FixedSize() const { return m_min_size == m_max_size; }
    unsigned int Size() const { return m_max_size; }

    unsigned int MinSize() const { return m_min_size; }
    unsigned int MaxSize() const { return m_max_size; }

    unsigned int FieldCount() const { return m_fields.size(); }
    const class FieldDescriptor *GetField(unsigned int index) const {
      if (index < m_fields.size())
        return m_fields[index];
      return NULL;
    }

    void Accept(FieldDescriptorVisitor &visitor) const;

  private:
    vector<class FieldDescriptor *> m_fields;
    uint8_t m_min_size, m_max_size;
};
}  // messaging
}  // ola

#endif  // INCLUDE_OLA_MESSAGING_DESCRIPTOR_H_
