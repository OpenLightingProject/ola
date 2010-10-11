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
 * JsonSections.cpp
 * This builds the json string for the web UI.
 * Copyright (C) 2010 Simon Newton
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "ola/web/JsonSections.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"


namespace ola {
namespace web {

using std::endl;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;



string GenericItem::AsString() const {
  stringstream output;
  output << "    {" << endl;
  if (!m_button_text.empty())
    output << "    \"button\": \"" << m_button_text << "\"," << endl;
  output << "    \"description\": \"" << m_description << "\"," << endl;
  if (!m_id.empty())
    output << "    \"id\": \"" << m_id << "\"," << endl;
  output << "    \"type\": \"" << Type() << "\"," << endl;
  output << "    \"value\": " << Value() << "," << endl;
  output << ExtraProperties();
  output << "    }," << endl;
  return output.str();
}


string UIntItem::ExtraProperties() const {
  stringstream output;
  if (m_min_set)
    output << "    \"min\": " << m_min << "," << endl;
  if (m_max_set)
    output << "    \"max\": " << m_max << "," << endl;
  return output.str();
}


void SelectItem::AddItem(const string &label, const string &value) {
  pair<string, string> p(label, value);
  m_values.push_back(p);
}


void SelectItem::AddItem(const string &label, unsigned int value) {
  AddItem(label, IntToString(value));
}

string SelectItem::Value() const {
  return "\"\"";
}


string SelectItem::ExtraProperties() const {
  stringstream output;
  output << "    \"selected_offset\": " << m_selected_offset << "," << endl;
  return output.str();
}


/**
 * Create a new section response
 */
JsonSection::JsonSection(bool allow_refresh)
    : m_allow_refresh(allow_refresh),
      m_error(""),
      m_save_button_text("") {
}


/**
 * Cleanup
 */
JsonSection::~JsonSection() {
  vector<const GenericItem*>::const_iterator iter = m_items.begin();
  for (; iter != m_items.end(); ++iter) {
    delete *iter;
  }
}


/**
 * Add an item to this section, ownership is transferred.
 */
void JsonSection::AddItem(const GenericItem *item) {
  m_items.push_back(item);
}


/*
 * Return the section as a string.
 */
string JsonSection::AsString() {
  stringstream output;
  output << "{" << endl;
  output << "  \"refresh\": " << m_allow_refresh << "," << endl;
  output << "  \"error\": \"" << m_error << "\"," << endl;
  if (!m_save_button_text.empty())
    output << "  \"save_button\": \"" << m_save_button_text << "\"," << endl;
  output << "  \"fields\": [" << endl;

  vector<const GenericItem*>::const_iterator iter = m_items.begin();
  for (; iter != m_items.end(); ++iter) {
    output << (*iter)->AsString();
  }
  output << "  ]," << endl;
  output << "}" << endl;
  return output.str();
}
}  // web
}  // ola
