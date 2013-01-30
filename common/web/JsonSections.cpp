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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "ola/web/Json.h"
#include "ola/StringUtils.h"


namespace ola {
namespace web {

using std::endl;
using std::pair;
using std::string;
using std::stringstream;
using std::vector;
using ola::EscapeString;


void GenericItem::PopulateItem(JsonObject *item) const {
  if (!m_button_text.empty())
    item->Add("button", m_button_text);
  if (!m_id.empty())
    item->Add("id", m_id);

  item->Add("description", m_description);
  item->Add("type", Type());

  SetValue(item);
  SetExtraProperties(item);
}


void UIntItem::SetExtraProperties(JsonObject *item) const {
  if (m_min_set)
    item->Add("min", m_min);
  if (m_max_set)
    item->Add("max", m_max);
}


void SelectItem::AddItem(const string &label, const string &value) {
  pair<string, string> p(label, value);
  m_values.push_back(p);
}


void SelectItem::AddItem(const string &label, unsigned int value) {
  AddItem(label, IntToString(value));
}


void SelectItem::SetValue(JsonObject *item) const {
  JsonArray *options = item->AddArray("value");
  vector<pair<string, string> >::const_iterator iter = m_values.begin();
  for (;iter != m_values.end(); ++iter) {
    JsonObject *option = options->AppendObject();
    option->Add("label", iter->first);
    option->Add("value", iter->second);
  }
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
string JsonSection::AsString() const {
  JsonObject json;

  json.Add("refresh", m_allow_refresh);
  json.Add("error", m_error);
  if (!m_save_button_text.empty())
    json.Add("save_button", m_save_button_text);

  JsonArray *items = json.AddArray("items");
  vector<const GenericItem*>::const_iterator iter = m_items.begin();
  for (; iter != m_items.end(); ++iter) {
    JsonObject *item = items->AppendObject();
    (*iter)->PopulateItem(item);
  }
  return JsonWriter::AsString(json);
}
}  // web
}  // ola
