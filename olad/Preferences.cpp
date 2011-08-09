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
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <list>
#include <map>
#include <memory>
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
  if (!StringToInt(value, &output))
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
    if (!StringToInt(tokens[i], &octet))
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



// FilePreferenceSaverThread
//-----------------------------------------------------------------------------
FilePreferenceSaverThread::FilePreferenceSaverThread() {
  // set a long poll interval so we don't spin
  m_ss.SetDefaultInterval(TimeInterval(60, 0));
}


void FilePreferenceSaverThread::SavePreferences(
    const string &file_name,
    const PreferencesMap &preferences) {
  const string *file_name_ptr = new string(file_name);
  const PreferencesMap *save_map = new PreferencesMap(preferences);
  SingleUseCallback0<void> *cb = NewSingleCallback(
                                 this,
                                 &FilePreferenceSaverThread::SaveToFile,
                                 file_name_ptr,
                                 save_map);
  m_ss.Execute(cb);
}


/**
 * Called by the new thread.
 */
void *FilePreferenceSaverThread::Run() {
  m_ss.Run();
  return NULL;
}


/**
 * Stop the saving thread
 */
bool FilePreferenceSaverThread::Join(void *ptr) {
  m_ss.Terminate();
  return OlaThread::Join(ptr);
}


/**
 * This can be used to syncronize with the file saving thread. Useful if you
 * want to make sure the files have been written to disk before continuing.
 * This blocks until all pending save requests are complete.
 */
void FilePreferenceSaverThread::Syncronize() {
  pthread_mutex_t syncronize_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
  pthread_mutex_lock(&syncronize_mutex);
  m_ss.Execute(NewSingleCallback(
        this,
        &FilePreferenceSaverThread::CompleteSyncronization,
        &condition_var));
  pthread_cond_wait(&condition_var, &syncronize_mutex);
}


/**
 * Perform the save
 */
void FilePreferenceSaverThread::SaveToFile(
    const string *filename_ptr,
    const PreferencesMap *pref_map_ptr) {
  std::auto_ptr<const string> filename(filename_ptr);
  std::auto_ptr<const PreferencesMap> pref_map(pref_map_ptr);

  PreferencesMap::const_iterator iter;
  ofstream pref_file(filename->data());

  if (!pref_file.is_open()) {
    OLA_INFO << "Could not open " << *filename_ptr << ": " << strerror(errno);
  }

  for (iter = pref_map->begin(); iter != pref_map->end(); ++iter) {
    pref_file << iter->first << " = " << iter->second << std::endl;
  }
  pref_file.close();
}


/**
 * Notify the blocked thread we're done
 */
void FilePreferenceSaverThread::CompleteSyncronization(
    pthread_cond_t *condition) {
  pthread_cond_signal(condition);
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
  m_saver_thread->SavePreferences(FileName(), m_pref_map);
  return true;
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
}  // ola
