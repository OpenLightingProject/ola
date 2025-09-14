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
    SchemaPrinter(bool include_intervals = true,
                  bool include_labels = true,
                  unsigned int indent_size = DEFAULT_INDENT)
        : m_include_intervals(include_intervals),
          m_include_labels(include_labels),
          m_indent(0),
          m_indent_size(indent_size) {
    }
    ~SchemaPrinter() {}

    bool Descend() const { return true; }
    std::string AsString() { return m_str.str(); }
    void Reset() { m_str.str(""); }

    void Visit(const BoolFieldDescriptor*);
    void Visit(const IPV4FieldDescriptor*);
    void Visit(const IPV6FieldDescriptor*);
    void Visit(const MACFieldDescriptor*);
    void Visit(const UIDFieldDescriptor*);
    void Visit(const StringFieldDescriptor*);
    void Visit(const UInt8FieldDescriptor*);
    void Visit(const UInt16FieldDescriptor*);
    void Visit(const UInt32FieldDescriptor*);
    void Visit(const UInt64FieldDescriptor*);
    void Visit(const Int8FieldDescriptor*);
    void Visit(const Int16FieldDescriptor*);
    void Visit(const Int32FieldDescriptor*);
    void Visit(const Int64FieldDescriptor*);
    void Visit(const FieldDescriptorGroup*);
    void PostVisit(const FieldDescriptorGroup*);

 private:
    bool m_include_intervals, m_include_labels;
    std::ostringstream m_str;
    unsigned int m_indent, m_indent_size;

    void AppendHeading(const std::string &name, const std::string &type);

    template<class vector_class>
    void MaybeAppendIntervals(const vector_class &intervals) {
      if (!m_include_intervals)
        return;
      typename vector_class::const_iterator iter = intervals.begin();
      for (; iter != intervals.end(); ++iter) {
        if (iter->first == iter->second) {
          m_str << (iter == intervals.begin() ? ": " : ", ") <<
            static_cast<int64_t>(iter->first);
        } else {
          m_str << (iter == intervals.begin() ? ": " : ", ") << "(" <<
            static_cast<int64_t>(iter->first) << ", " <<
            static_cast<int64_t>(iter->second) << ")";
        }
      }
    }

    template<class map_class>
    void MaybeAppendLabels(const map_class &labels) {
      if (!m_include_labels)
        return;
      typename map_class::const_iterator iter = labels.begin();
      for (; iter != labels.end(); ++iter) {
        m_str << std::endl << std::string(m_indent + m_indent_size, ' ') <<
            iter->first << ": " << static_cast<int64_t>(iter->second);
      }
    }

    static const unsigned int DEFAULT_INDENT = 2;
};
}  // namespace messaging
}  // namespace ola
#endif  // INCLUDE_OLA_MESSAGING_SCHEMAPRINTER_H_
