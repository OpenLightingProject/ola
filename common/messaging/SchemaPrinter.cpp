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
 * SchemaPrinter.cpp
 * Prints the text representation of a schema.
 * Copyright (C) 2011 Simon Newton
 */


#include <ola/messaging/Descriptor.h>
#include <ola/messaging/SchemaPrinter.h>
#include <iostream>
#include <string>

namespace ola {
namespace messaging {


using std::string;
using std::endl;


void SchemaPrinter::Visit(const BoolFieldDescriptor *descriptor) {
  m_str << string(m_indent, ' ') << descriptor->Name() << ": bool" << endl;
}


void SchemaPrinter::Visit(const IPV4FieldDescriptor *descriptor) {
  m_str << string(m_indent, ' ') << descriptor->Name() << ": IPv4 address"
        << endl;
}


void SchemaPrinter::Visit(const IPV6FieldDescriptor *descriptor) {
  m_str << string(m_indent, ' ') << descriptor->Name() << ": IPv6 address"
        << endl;
}


void SchemaPrinter::Visit(const MACFieldDescriptor *descriptor) {
  m_str << string(m_indent, ' ') << descriptor->Name() << ": MAC" << endl;
}


void SchemaPrinter::Visit(const UIDFieldDescriptor *descriptor) {
  m_str << string(m_indent, ' ') << descriptor->Name() << ": UID" << endl;
}


void SchemaPrinter::Visit(const StringFieldDescriptor *descriptor) {
  m_str << string(m_indent, ' ') << descriptor->Name() << ": string [" <<
    descriptor->MinSize() << ", " << descriptor->MaxSize() << "]" << endl;
}


void SchemaPrinter::Visit(const UInt8FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "uint8");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const UInt16FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "uint16");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const UInt32FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "uint32");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const UInt64FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "uint64");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const Int8FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "int8");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const Int16FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "int16");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const Int32FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "int32");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const Int64FieldDescriptor *descriptor) {
  AppendHeading(descriptor->Name(), "int64");
  MaybeAppendIntervals(descriptor->Intervals());
  MaybeAppendLabels(descriptor->Labels());
  m_str << endl;
}


void SchemaPrinter::Visit(const FieldDescriptorGroup *descriptor) {
  m_str << string(m_indent, ' ') << descriptor->Name() << " {" << endl;
  m_indent += m_indent_size;
}


void SchemaPrinter::PostVisit(const FieldDescriptorGroup *descriptor) {
  m_indent -= m_indent_size;
  m_str << string(m_indent, ' ') << "}" << endl;
  (void) descriptor;
}


void SchemaPrinter::AppendHeading(const string &name, const string &type) {
  m_str << string(m_indent, ' ') << name << ": " << type;
}
}  // namespace messaging
}  // namespace ola
