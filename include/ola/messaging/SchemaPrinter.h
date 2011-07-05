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
 * SchemaPrinter.h
 * Builds a string which contains the text representation of the schema.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_SCHEMAPRINTER_H_
#define INCLUDE_OLA_MESSAGING_SCHEMAPRINTER_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <string>
#include <sstream>

namespace ola {
namespace messaging {


/**
 * This visitor prints the schema as a string.
 */
class SchemaPrinter: public FieldDescriptorVisitor {
  public:
    SchemaPrinter(bool include_range,
                  bool include_labels,
                  unsigned int indent_size = DEFAULT_INDENT)
        : m_include_range(include_range),
          m_include_labels(include_labels),
          m_indent(0),
          m_indent_size(indent_size) {
    }
    ~SchemaPrinter() {}

    string AsString() { return m_str.str(); }
    void Reset() { m_str.str(""); }

    void Visit(const BoolFieldDescriptor*);
    void Visit(const StringFieldDescriptor*);
    void Visit(const IntegerFieldDescriptor<uint8_t>*);
    void Visit(const IntegerFieldDescriptor<uint16_t>*);
    void Visit(const IntegerFieldDescriptor<uint32_t>*);
    void Visit(const IntegerFieldDescriptor<int8_t>*);
    void Visit(const IntegerFieldDescriptor<int16_t>*);
    void Visit(const IntegerFieldDescriptor<int32_t>*);
    void Visit(const GroupFieldDescriptor*);
    void PostVisit(const GroupFieldDescriptor*);

  private:
    bool m_include_range, m_include_labels;
    std::stringstream m_str;
    unsigned int m_indent, m_indent_size;

    static const unsigned int DEFAULT_INDENT = 2;
};
}  // messaging
}  // ola
#endif  // INCLUDE_OLA_MESSAGING_SCHEMAPRINTER_H_
