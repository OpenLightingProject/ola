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
 * StringMessageBuilder.h
 * Builds a Message object from a list of strings & a Descriptor.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_STRINGMESSAGEBUILDER_H_
#define INCLUDE_OLA_RDM_STRINGMESSAGEBUILDER_H_

#include <ola/messaging/DescriptorVisitor.h>
#include <stack>
#include <string>
#include <vector>

namespace ola {

namespace messaging {
  class Descriptor;
  class MessageFieldInterface;
  class Message;
}

namespace rdm {

using std::string;
using std::vector;

/**
 * This visitor builds a message based on a vector of strings from a
 * Descriptor.
 */
class StringMessageBuilder: public ola::messaging::FieldDescriptorVisitor {
  public:
    StringMessageBuilder();
    ~StringMessageBuilder();

    // we handle decending into groups ourself
    bool Descend() const { return false; }
    const ola::messaging::Message *GetMessage(
        const vector<string> &inputs,
        const class ola::messaging::Descriptor*);
    const string GetError() const { return m_error_string; }

    void Visit(const ola::messaging::BoolFieldDescriptor*);
    void Visit(const ola::messaging::StringFieldDescriptor*);
    void Visit(const ola::messaging::UInt8FieldDescriptor*);
    void Visit(const ola::messaging::UInt16FieldDescriptor*);
    void Visit(const ola::messaging::UInt32FieldDescriptor*);
    void Visit(const ola::messaging::Int8FieldDescriptor*);
    void Visit(const ola::messaging::Int16FieldDescriptor*);
    void Visit(const ola::messaging::Int32FieldDescriptor*);
    void Visit(const ola::messaging::FieldDescriptorGroup*);
    void PostVisit(const ola::messaging::FieldDescriptorGroup*);

  private:
    vector<string> m_inputs;
    std::stack<vector<const ola::messaging::MessageFieldInterface*> > m_groups;
    unsigned int m_offset, m_input_size, m_group_instance_count;
    bool m_error;
    string m_error_string;

    bool StopParsing() const;
    void SetError(const string &error);

    template<typename type>
    void VisitInt(const ola::messaging::IntegerFieldDescriptor<type> *);

    void InitVars(const vector<string> &inputs);
    void CleanUpVector();
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_STRINGMESSAGEBUILDER_H_
