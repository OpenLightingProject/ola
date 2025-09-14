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
 * MessagePrinter.h
 * Print the contents of a message.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_MESSAGEPRINTER_H_
#define INCLUDE_OLA_MESSAGING_MESSAGEPRINTER_H_

#include <ola/messaging/MessageVisitor.h>
#include <string>
#include <sstream>

namespace ola {
namespace messaging {


/**
 * The base class for all message printers.
 */
class MessagePrinter: public MessageVisitor {
 public:
    virtual ~MessagePrinter() {}

    std::string AsString(const class Message *message);

    virtual void Visit(const BoolMessageField*) {}
    virtual void Visit(const IPV4MessageField*) {}
    virtual void Visit(const IPV6MessageField*) {}
    virtual void Visit(const MACMessageField*) {}
    virtual void Visit(const UIDMessageField*) {}
    virtual void Visit(const StringMessageField*) {}
    virtual void Visit(const BasicMessageField<uint8_t>*) {}
    virtual void Visit(const BasicMessageField<uint16_t>*) {}
    virtual void Visit(const BasicMessageField<uint32_t>*) {}
    virtual void Visit(const BasicMessageField<uint64_t>*) {}
    virtual void Visit(const BasicMessageField<int8_t>*) {}
    virtual void Visit(const BasicMessageField<int16_t>*) {}
    virtual void Visit(const BasicMessageField<int32_t>*) {}
    virtual void Visit(const BasicMessageField<int64_t>*) {}
    virtual void Visit(const GroupMessageField*) {}
    virtual void PostVisit(const GroupMessageField*) {}

 protected:
    std::ostringstream& Stream() { return m_str; }
    virtual void PostStringHook() {}
    virtual std::string TransformLabel(const std::string &label) {
      return label;
    }

 private:
    std::ostringstream m_str;
};


/**
 * The generic printer returns key: value fields.
 */
class GenericMessagePrinter: public MessagePrinter {
 public:
    GenericMessagePrinter(unsigned int indent_size = DEFAULT_INDENT,
                          unsigned int initial_indent = 0)
        : m_indent(initial_indent),
          m_indent_size(indent_size) {
    }
    ~GenericMessagePrinter() {}

    virtual void Visit(const BoolMessageField*);
    virtual void Visit(const IPV4MessageField*);
    virtual void Visit(const IPV6MessageField*);
    virtual void Visit(const MACMessageField*);
    virtual void Visit(const UIDMessageField*);
    virtual void Visit(const StringMessageField*);
    virtual void Visit(const BasicMessageField<uint8_t>*);
    virtual void Visit(const BasicMessageField<uint16_t>*);
    virtual void Visit(const BasicMessageField<uint32_t>*);
    virtual void Visit(const BasicMessageField<uint64_t>*);
    virtual void Visit(const BasicMessageField<int8_t>*);
    virtual void Visit(const BasicMessageField<int16_t>*);
    virtual void Visit(const BasicMessageField<int32_t>*);
    virtual void Visit(const BasicMessageField<int64_t>*);
    virtual void Visit(const GroupMessageField*);
    virtual void PostVisit(const GroupMessageField*);

    static const unsigned int DEFAULT_INDENT = 2;

 private:
    unsigned int m_indent, m_indent_size;

    void AppendUInt(const std::string &name,
                    uint64_t value,
                    const std::string &label,
                    int8_t multiplier);
    void AppendInt(const std::string &name,
                   int64_t value,
                   const std::string &label,
                   int8_t multiplier);
    void AppendMultiplier(int8_t multiplier);
};
}  // namespace messaging
}  // namespace ola
#endif  // INCLUDE_OLA_MESSAGING_MESSAGEPRINTER_H_
