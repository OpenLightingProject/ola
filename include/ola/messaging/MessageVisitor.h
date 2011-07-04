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
 * MessageVisitor.h
 * The interface for the class which visits Message & MessageFields
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_MESSAGING_MESSAGEVISITOR_H_
#define INCLUDE_OLA_MESSAGING_MESSAGEVISITOR_H_


namespace ola {
namespace messaging {


class StringMessageField;
class GroupMessageField;

template <typename type>
class BasicMessageField;

/**
 * The interface for the Message Visitor
 */
class FieldDescriptorVisitor {
  public:
    virtual ~FieldDescriptorVisitor() {}

    virtual void Visit(const IntegerFieldDescriptor<bool>*) = 0;
    virtual void Visit(const StringMessageField*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<uint8_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<uint16_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<uint32_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<int8_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<int16_t>*) = 0;
    virtual void Visit(const IntegerFieldDescriptor<int32_t>*) = 0;
    virtual void Visit(const GroupMessageField*) = 0;
    virtual void PostVisit(const GroupMessageField*) = 0;
};
}  // messaging
}  // ola
#endif  // INCLUDE_OLA_MESSAGING_MESSAGEVISITOR_H_
