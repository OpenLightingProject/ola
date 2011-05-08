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
 * Preferences.cpp
 * This class stores preferences in files
 * Copyright (C) 2005-2008 Simon Newton
 */
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include <fstream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "olad/Preferences.h"

namespace ola {

using std::ifstream;
using std::ofstream;
using std::pair;

const char BoolValidator::ENABLED[] = "true";
const char BoolValidator::DISABLED[] = "false";

const char FileBackedPreferences::OLA_CONFIG_PREFIX[] = "ola-";
const char FileBackedPreferences::OLA_CONFIG_SUFFIX[] = ".conf";

// Validators
//-----------------------------------------------------------------------------

bool StringValidator::IsValid(const string &value) const {
  return !value.empty();
}


bool BoolValidator::IsValid(const string &value) const {
  return (value == ENABLED || value == DISABLED);
}


bool IntValidator::IsValid(const string &value) const {
  unsigned int output;
  if (!StringToUInt(value, &output))
    return false;

  return (output >= m_gt && output <= m_lt);
}


bool SetValidator::IsValid(const string &value) const {
  return m_values.end() != m_values.find(value);
}


bool IPv4Validator::IsValid(const string &value) const {
  if (value.empty())
    return m_empty_ok;

  vector<string> tokens;
  StringSplit(value, tokens, ".");
  if (tokens.size() != 4)
    return false;

  for (unsigned int i = 0 ; i < 4; i++) {
    unsigned int octet;
    if (!StringToUInt(tokens[i], &octet))
      return false;
    if (octet > 255)
      return false;
  }
  return true;
}


// Prefs Factory
//-----------------------------------------------------------------------------

/**
 * Cleanup
 */
PreferencesFactory::~PreferencesFactory() {
  map<string, Preferences*>::const_iterator iter;
  for (iter = m_preferences_map.begin(); iter != m_preferences_map.end();
       ++iter) {
    delete iter->second;
  }
  m_preferences_map.clear();
}


/**
 * Lookup a preference object
 */
Preferences *PreferencesFactory::NewPreference(const string &name) {
  map<string, Preferences*>::iterator iter = m_preferences_map.find(name);
  if (iter == m_preferences_map.end()) {
    Preferences *pref = Create(name);
    m_preferences_map.insert(pair<string, Preferences*>(name, pref));
    return pref;
  } else {
    return iter->second;
  }
}


// Memory Preferences
//-----------------------------------------------------------------------------

/*
 * Destroy this object
 */
MemoryPreferences::~MemoryPreferences() {
  m_pref_map.clear();
}


/*
 * Clear the preferences
 */
void MemoryPreferences::Clear() {
  m_pref_map.clear();
}


/*
 * Set a preference value, overiding the existing value.
 * @param key
 * @param value
 */
void MemoryPreferences::SetValue(const string &key, const string &value) {
  m_pref_map.erase(key);
  m_pref_map.insert(pair<string, string>(key, value));
}


/*
 * Adds this preference value to the store
 * @param key
 * @param value
 */
void MemoryPreferences::SetMultipleValue(const string &key,
                                         const string &value) {
  m_pref_map.insert(pair<string, string>(key, value));
}


/*
 * Set a preference value only if it doesn't pass the validator.
 * Note this only checks the first value.
 * @param key
 * @param validator A Validator object
 * @param value the new value
 * @return true if we set the value, false if it already existed
 */
bool MemoryPreferences::SetDefaultValue(const string &key,
                                        const Validator &validator,
                                        const string &value) {
  map<string, string>::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter == m_pref_map.end() || !validator.IsValid(iter->second)) {
    SetValue(key, value);
    return true;
  }
  return false;
}


/*
 * Get a preference value
 * @param key the key to fetch
 * @return the value corrosponding to key, or the empty string if the key
 * doesn't exist.
 */
string MemoryPreferences::GetValue(const string &key) const {
  map<string, string>::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter != m_pref_map.end())
    return iter->second;
  return "";
}


/*
 * Returns all preference values corrosponding to this key
 * @returns a vector of strings.
 */
vector<string> MemoryPreferences::GetMultipleValue(const string &key) const {
  vector<string> values;
  map<string, string>::const_iterator iter;

  for (iter = m_pref_map.find(key);
       iter != m_pref_map.end() && iter->first == key; ++iter) {
    values.push_back(iter->second);
  }
  return values;
}


/*
 * Remove a preference value.
 * @param key
 */
void MemoryPreferences::RemoveValue(const string &key) {
  m_pref_map.erase(key);
}


/*
 * Set a value as a bool.
 * @param key
 * @param value
 */
void MemoryPreferences::SetValueAsBool(const string &key, bool value) {
  m_pref_map.erase(key);
  if (value)
    m_pref_map.insert(pair<string, string>(key, BoolValidator::ENABLED));
  else
    m_pref_map.insert(pair<string, string>(key, BoolValidator::DISABLED));
}


/*
 * Get a preference value as a bool
 * @param key the key to fetch
 * @return true if the value is 'true' or false otherwise
 */
bool MemoryPreferences::GetValueAsBool(const string &key) const {
  map<string, string>::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter != m_pref_map.end())
    return iter->second == BoolValidator::ENABLED;
  return false;
}


// FileBackedPreferences
//-----------------------------------------------------------------------------

/*
 * Load the preferences from storage
 */
bool FileBackedPreferences::Load() {
  return LoadFromFile(FileName());
}


/*
 * Save the preferences to storage
 */
bool FileBackedPreferences::Save() const {
  return SaveToFile(FileName());
}


/*
 * Return the name of the file used to save the preferences
 */
const string FileBackedPreferences::FileName() const {
  return (m_directory + "/" + OLA_CONFIG_PREFIX + m_preference_name +
          OLA_CONFIG_SUFFIX);
}


/*
 * Load these preferences from a file
 * @param filename the filename to load from
 */
bool FileBackedPreferences::LoadFromFile(const string &filename) {
  ifstream pref_file(filename.data());

  if (!pref_file.is_open()) {
    OLA_INFO << "Missing " << filename << ": " << strerror(errno) <<
      " - this isn't an error, we'll just use the defaults";
    return false;
  }

  m_pref_map.clear();
  string line;
  while (getline(pref_file, line)) {
    StringTrim(&line);

    if (line.empty() || line.at(0) == '#')
      continue;

    vector<string> tokens;
    StringSplit(line, tokens, "=");

    if (tokens.size() != 2) {
      OLA_INFO << "Skipping line: " << line;
      continue;
    }

    string key = tokens[0];
    string value = tokens[1];
    StringTrim(&key);
    StringTrim(&value);
    m_pref_map.insert(pair<string, string>(key, value));
  }
  pref_file.close();
  return true;
}


/*
 * Save the preferences to a file
 * @param filename - the name of the file to save to
 */
bool FileBackedPreferences::SaveToFile(const string &filename) const {
  map<string, string>::const_iterator iter;
  ofstream pref_file(filename.data());

  if (!pref_file.is_open()) {
    OLA_INFO << "Missing " << filename << ": " << strerror(errno);
    return false;
  }

  for (iter = m_pref_map.begin(); iter != m_pref_map.end(); ++iter) {
    pref_file << iter->first << " = " << iter->second << std::endl;
  }

  pref_file.close();
  return true;
}
}  // ola
