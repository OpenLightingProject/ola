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
    explicit StringMessageBuilder(const vector<string> &inputs)
        : m_inputs(inputs),
          m_offset(0),
          m_input_size(inputs.size()),
          m_error(false) {
    }
    ~StringMessageBuilder() {}

    const ola::messaging::Message *GetMessage() const;
    const string GetError() const { return m_error_string; }

    void Visit(const ola::messaging::BoolFieldDescriptor*);
    void Visit(const ola::messaging::StringFieldDescriptor*);
    void Visit(const ola::messaging::UInt8FieldDescriptor*);
    void Visit(const ola::messaging::UInt16FieldDescriptor*);
    void Visit(const ola::messaging::UInt32FieldDescriptor*);
    void Visit(const ola::messaging::Int8FieldDescriptor*);
    void Visit(const ola::messaging::Int16FieldDescriptor*);
    void Visit(const ola::messaging::Int32FieldDescriptor*);
    void Visit(const ola::messaging::GroupFieldDescriptor*);
    void PostVisit(const ola::messaging::GroupFieldDescriptor*);

  private:
    const vector<string> m_inputs;
    vector<const ola::messaging::MessageFieldInterface*> m_messages;
    std::stack<vector<const ola::messaging::MessageFieldInterface*> > m_groups;
    unsigned int m_offset, m_input_size;
    bool m_error;
    string m_error_string;

    bool StopParsing() const;
    void SetError(const string &error);

    template<typename type>
    void VisitInt(const ola::messaging::IntegerFieldDescriptor<type> *);
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_STRINGMESSAGEBUILDER_H_
