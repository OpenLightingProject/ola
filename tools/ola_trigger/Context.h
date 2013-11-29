/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Context.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef TOOLS_OLA_TRIGGER_CONTEXT_H_
#define TOOLS_OLA_TRIGGER_CONTEXT_H_

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdint.h>
#include <sstream>
#include <string>
#include HASH_MAP_H

using std::string;


/**
 * A context is a collection of variables and their values.
 */
class Context {
 public:
    Context() {}
    ~Context();

    bool Lookup(const string &name, string *value) const;
    void Update(const string &name, const string &value);

    void SetSlotValue(uint8_t value);
    void SetSlotOffset(uint16_t offset);

    string AsString() const;
    friend std::ostream& operator<<(std::ostream &out, const Context&);

    static const char SLOT_VALUE_VARIABLE[];
    static const char SLOT_OFFSET_VARIABLE[];

 private:
    typedef HASH_NAMESPACE::HASH_MAP_CLASS<string, string> VariableMap;
    VariableMap m_variables;
};
#endif  // TOOLS_OLA_TRIGGER_CONTEXT_H_
