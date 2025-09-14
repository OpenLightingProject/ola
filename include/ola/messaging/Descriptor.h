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
 * Descriptor.h
 * Holds the metadata (schema) for a Message.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_DESCRIPTOR_H_
#define INCLUDE_OLA_MESSAGING_DESCRIPTOR_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <ola/network/IPV4Address.h>
#include <ola/network/IPV6Address.h>
#include <ola/network/MACAddress.h>
#include <ola/rdm/UID.h>
#include <map>
#include <string>
#include <vector>
#include <utility>

namespace ola {
namespace messaging {

class FieldDescriptorVisitor;

/**
 * Describes a field, which may be a group of sub fields.
 */
class FieldDescriptorInterface {
 public:
    virtual ~FieldDescriptorInterface() {}

    // Returns the name of this field
    virtual const std::string& Name() const = 0;

    // Call back into a FieldDescriptorVisitor
    virtual void Accept(FieldDescriptorVisitor *visitor) const = 0;

    // Returns true if the size of this field is constant
    virtual bool FixedSize() const = 0;

    // True if there is some bound on the field's size.
    virtual bool LimitedSize() const = 0;

    // This is the max size in bytes of the field. This is only valid if
    // LimitedSize() is  true, otherwise it returns 0.
    virtual unsigned int MaxSize() const = 0;
};


/**
 * The base implementation of a field.
 */
class FieldDescriptor: public FieldDescriptorInterface {
 public:
    explicit FieldDescriptor(const std::string &name)
        : m_name(name) {
    }
    virtual ~FieldDescriptor() {}

    // Returns the name of this field
    const std::string& Name() const { return m_name; }

 private:
    std::string m_name;
};


/**
 * A FieldDescriptor that represents a bool
 */
class BoolFieldDescriptor: public FieldDescriptor {
 public:
    explicit BoolFieldDescriptor(const std::string &name)
        : FieldDescriptor(name) {
    }

    bool FixedSize() const { return true; }
    bool LimitedSize() const { return true; }
    unsigned int MaxSize() const { return 1; }

    void Accept(FieldDescriptorVisitor *visitor) const {
      visitor->Visit(this);
    }
};


/**
 * A FieldDescriptor that represents a IPv4 Address
 */
class IPV4FieldDescriptor: public FieldDescriptor {
 public:
    explicit IPV4FieldDescriptor(const std::string &name)
        : FieldDescriptor(name) {
    }

    bool FixedSize() const { return true; }
    bool LimitedSize() const { return true; }
    unsigned int MaxSize() const { return ola::network::IPV4Address::LENGTH; }

    void Accept(FieldDescriptorVisitor *visitor) const {
      visitor->Visit(this);
    }
};


/**
 * A FieldDescriptor that represents a IPv6 Address
 */
class IPV6FieldDescriptor: public FieldDescriptor {
 public:
    explicit IPV6FieldDescriptor(const std::string &name)
        : FieldDescriptor(name) {
    }

    bool FixedSize() const { return true; }
    bool LimitedSize() const { return true; }
    unsigned int MaxSize() const { return ola::network::IPV6Address::LENGTH; }

    void Accept(FieldDescriptorVisitor *visitor) const {
      visitor->Visit(this);
    }
};


/**
 * A FieldDescriptor that represents a MAC Address
 */
class MACFieldDescriptor: public FieldDescriptor {
 public:
    explicit MACFieldDescriptor(const std::string &name)
        : FieldDescriptor(name) {
    }

    bool FixedSize() const { return true; }
    bool LimitedSize() const { return true; }
    unsigned int MaxSize() const { return ola::network::MACAddress::LENGTH; }

    void Accept(FieldDescriptorVisitor *visitor) const {
      visitor->Visit(this);
    }
};


/**
 * A FieldDescriptor that represents a UID
 */
class UIDFieldDescriptor: public FieldDescriptor {
 public:
    explicit UIDFieldDescriptor(const std::string &name)
        : FieldDescriptor(name) {
    }

    bool FixedSize() const { return true; }
    bool LimitedSize() const { return true; }
    unsigned int MaxSize() const { return ola::rdm::UID::LENGTH; }

    void Accept(FieldDescriptorVisitor *visitor) const {
      visitor->Visit(this);
    }
};


/**
 * A FieldDescriptor that represents a string
 */
class StringFieldDescriptor: public FieldDescriptor {
 public:
    StringFieldDescriptor(const std::string &name,
                          uint8_t min_size,
                          uint8_t max_size)
        : FieldDescriptor(name),
          m_min_size(min_size),
          m_max_size(max_size) {
    }

    bool FixedSize() const { return m_min_size == m_max_size; }
    bool LimitedSize() const { return true; }
    unsigned int MinSize() const { return m_min_size; }
    unsigned int MaxSize() const { return m_max_size; }

    void Accept(FieldDescriptorVisitor *visitor) const {
      visitor->Visit(this);
    }

 private:
    uint8_t m_min_size, m_max_size;
};


/**
 * A FieldDescriptor that represents an integer type.
 *
 * Intervals are closed (include the endpoints).
 */
template <typename type>
class IntegerFieldDescriptor: public FieldDescriptor {
 public:
    typedef std::pair<type, type> Interval;
    typedef std::vector<std::pair<type, type> > IntervalVector;
    typedef std::map<std::string, type> LabeledValues;

    IntegerFieldDescriptor(const std::string &name,
                           bool little_endian = false,
                           int8_t multiplier = 0)
        : FieldDescriptor(name),
          m_little_endian(little_endian),
          m_multiplier(multiplier) {
    }

    IntegerFieldDescriptor(const std::string &name,
                           const IntervalVector &intervals,
                           const LabeledValues &labels,
                           bool little_endian = false,
                           int8_t multiplier = 0)
        : FieldDescriptor(name),
          m_little_endian(little_endian),
          m_multiplier(multiplier),
          m_intervals(intervals),
          m_labels(labels) {
    }

    bool FixedSize() const { return true; }
    bool LimitedSize() const { return true; }
    unsigned int MaxSize() const { return sizeof(type); }
    int8_t Multiplier() const { return m_multiplier; }
    bool IsLittleEndian() const { return m_little_endian; }

    const IntervalVector &Intervals() const { return m_intervals; }

    bool IsValid(type value) const {
      if (m_intervals.empty())
        return true;

      typename IntervalVector::const_iterator iter = m_intervals.begin();
      for (; iter != m_intervals.end(); ++iter) {
        if (value >= iter->first && value <= iter->second)
          return true;
      }
      return false;
    }

    const LabeledValues &Labels() const { return m_labels; }

    bool LookupLabel(const std::string &label, type *value) const {
      typename LabeledValues::const_iterator iter = m_labels.find(label);
      if (iter == m_labels.end())
        return false;
      *value = iter->second;
      return true;
    }

    const std::string LookupValue(type value) const {
      typename LabeledValues::const_iterator iter = m_labels.begin();
      for (; iter != m_labels.end(); ++iter) {
        if (iter->second == value)
          return iter->first;
      }
      return "";
    }

    void Accept(FieldDescriptorVisitor *visitor) const {
      visitor->Visit(this);
    }

 private:
    bool m_little_endian;
    int8_t m_multiplier;
    IntervalVector m_intervals;
    LabeledValues m_labels;
};


typedef IntegerFieldDescriptor<uint8_t> UInt8FieldDescriptor;
typedef IntegerFieldDescriptor<uint16_t> UInt16FieldDescriptor;
typedef IntegerFieldDescriptor<uint32_t> UInt32FieldDescriptor;
typedef IntegerFieldDescriptor<uint64_t> UInt64FieldDescriptor;
typedef IntegerFieldDescriptor<int8_t> Int8FieldDescriptor;
typedef IntegerFieldDescriptor<int16_t> Int16FieldDescriptor;
typedef IntegerFieldDescriptor<int32_t> Int32FieldDescriptor;
typedef IntegerFieldDescriptor<int64_t> Int64FieldDescriptor;


/**
 * A FieldDescriptor that consists of a group of FieldDescriptors. Groups can
 * vary in size two ways. First, the group may contain a field which itself is
 * of variable size (i.e. a string or another group). This type of message
 * structure requires some other data in the message itself to indicate the
 * field/group length and as such isn't supported.
 *
 * An example of this type of group would be:
 *
 * @verbatim
 * +----------------+
 * |    bool (1)    |
 * +----------------+
 * | string (0, 32) |
 * +----------------+
 * @endverbatim
 *
 *  This could hold data like:
 *    (true, "foo"),
 *    (false, "bar)
 *
 * The second (and simpler) type is where the group size is fixed (i.e.
 * contains only fixed length fields) and the number of times the group appears
 * in the message varies. By knowing the length of the message we can work out
 * the number of times a group occurs (see VariableFieldSizeCalculator.h which
 * does this).
 *
 * An example of this type of group would be:
 *
 * @verbatim
 * +----------------+
 * |    bool (1)    |
 * +----------------+
 * |   uint16 (2)   |
 * +----------------+
 * @endverbatim
 *
 *  This could hold data like:
 *    (true, 1000),
 *    (false, 34)
 *
 * DescriptorConsistencyChecker.h checks that a descriptor is the second type,
 * that is contains at most one variable-sized field.
 *
 * We refer to the datatypes within a group as fields, the actual
 * instantiations of a group as blocks. In the examples above, bool, string and
 * uint16 are the fields (represented by FieldDescriptorInterface objects) and
 * (true, "foo") & (true, 1000) are the blocks.
 */
class FieldDescriptorGroup: public FieldDescriptor {
 public:
    static const int16_t UNLIMITED_BLOCKS;

    FieldDescriptorGroup(const std::string &name,
                         const std::vector<const FieldDescriptor*> &fields,
                         uint16_t min_blocks,
                         int16_t max_blocks)
      : FieldDescriptor(name),
        m_fields(fields),
        m_min_blocks(min_blocks),
        m_max_blocks(max_blocks),
        m_populated(false),
        m_fixed_size(true),
        m_limited_size(true),
        m_block_size(0),
        m_max_block_size(0) {
    }
    virtual ~FieldDescriptorGroup();

    // This is true iff all fields in a group are of a fixed size and the
    // number of blocks is fixed
    bool FixedSize() const { return FixedBlockSize() && FixedBlockCount(); }

    // True if the number of blocks has some bound, and all fields also have
    // some bound.
    bool LimitedSize() const;

    // This is the max size of the group, which is only valid if LimitedSize()
    // is  true, otherwise it returns 0.
    unsigned int MaxSize() const;

    // Field information
    // the number of fields in this group
    unsigned int FieldCount() const { return m_fields.size(); }
    // True if all the fields in this group are a fixed size. This is then a
    // type 2 group as described above.
    bool FixedBlockSize() const;
    // If this block size is fixed, this returns the size of a single block,
    // otherwise it returns 0;
    unsigned int BlockSize() const;
    // If this block size is bounded, this returns the size of the block.
    unsigned int MaxBlockSize() const;

    // Blocks
    // The minimum number of blocks, usually 0 or 1.
    uint16_t MinBlocks() const { return m_min_blocks; }
    // A max size of UNLIMITED_BLOCKS means no restrictions on the number of
    // blocks
    int16_t MaxBlocks() const { return m_max_blocks; }
    // True if the block count is fixed.
    bool FixedBlockCount() const { return m_min_blocks == m_max_blocks; }


    const class FieldDescriptor *GetField(unsigned int index) const {
      if (index < m_fields.size())
        return m_fields[index];
      return NULL;
    }

    virtual void Accept(FieldDescriptorVisitor *visitor) const;

 protected:
    std::vector<const class FieldDescriptor *> m_fields;

 private:
    uint16_t m_min_blocks;
    int16_t m_max_blocks;
    mutable bool m_populated;
    mutable bool m_fixed_size, m_limited_size;
    mutable unsigned int m_block_size, m_max_block_size;

    void PopulateIfRequired() const;
};


/**
 * A descriptor is a group of fields which can't be repeated
 */
class Descriptor: public FieldDescriptorGroup {
 public:
    Descriptor(const std::string &name,
               const std::vector<const FieldDescriptor*> &fields)
        : FieldDescriptorGroup(name, fields, 1, 1) {}

    void Accept(FieldDescriptorVisitor *visitor) const;
};
}  // namespace messaging
}  // namespace ola

#endif  // INCLUDE_OLA_MESSAGING_DESCRIPTOR_H_
