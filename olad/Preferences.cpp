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

#include <list>
#include <fstream>

#include <ola/Logging.h>
#include <ola/StringUtils.h>
#include <olad/Preferences.h>

namespace ola {

using std::ifstream;
using std::ofstream;
using std::pair;

const string FileBackedPreferences::OLA_CONFIG_DIR = ".ola";
const string FileBackedPreferences::OLA_CONFIG_PREFIX = "ola-";
const string FileBackedPreferences::OLA_CONFIG_SUFFIX = ".conf";

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
  m_pref_map.insert(pair<string,string>(key, value));
}


/*
 * Set a preference value only if it's empty.
 * @param key
 * @param value
 * @return true if we set the value, false if it already existed
 */
bool MemoryPreferences::SetDefaultValue(const string &key,
                                        const string &value) {
  map<string, string>::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter == m_pref_map.end()) {
    SetValue(key, value);
    return true;
  }
  return false;
}


/*
 * Remove a preference value.
 * @param key
 */
void MemoryPreferences::RemoveValue(const string &key) {
  m_pref_map.erase(key);
}


/*
 * Adds this preference value to the store
 * @param key
 * @param value
 */
void MemoryPreferences::SetMultipleValue(const string &key,
                                         const string &value) {
  m_pref_map.insert(pair<string,string>(key, value));
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
    return iter->second.c_str();
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


// FileBackedPreferences
//-----------------------------------------------------------------------------

/*
 * Load the preferences from storage
 */
bool FileBackedPreferences::Load() {
  if (!ChangeDir())
    return false;
  return LoadFromFile(FileName());
}


/*
 * Save the preferences to storage
 */
bool FileBackedPreferences::Save() const {
  if (!ChangeDir())
    return false;

  return SaveToFile(FileName());
}


/*
 * Return the name of the file used to save the preferences
 */
const string FileBackedPreferences::FileName() const {
  return OLA_CONFIG_PREFIX + m_preference_name + OLA_CONFIG_SUFFIX;
}


/*
 * Load these preferences from a file
 * @param filename the filename to load from
 */
bool FileBackedPreferences::LoadFromFile(const string &filename) {
  ifstream pref_file(filename.data());

  if (!pref_file.is_open()) {
    OLA_INFO << "Missing " << filename << ": " << strerror(errno);
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
    m_pref_map.insert(pair<string,string>(key, value));
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

  for (iter=m_pref_map.begin(); iter != m_pref_map.end(); ++iter) {
    pref_file << iter->first << " = " << iter->second << std::endl;
  }

  pref_file.close();
  return true;
}


/*
 * Change to the ola preferences directory
 */
bool FileBackedPreferences::ChangeDir() const {
  struct passwd *ptr = getpwuid(getuid());
  if (ptr == NULL)
    return false;

  if (chdir(ptr->pw_dir))
    return false;

  if (chdir(OLA_CONFIG_DIR.data())) {
    // try and create it
    if (mkdir(OLA_CONFIG_DIR.data(), 0755))
      return false;

    if (chdir(OLA_CONFIG_DIR.data()))
      return false;
  }
  return true;
}


} //ola
