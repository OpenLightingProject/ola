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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Preferences.cpp
 * This class stores preferences in files
 * Copyright (C) 2005 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include "olad/Preferences.h"

#include <dirent.h>
#include <errno.h>
#ifdef _WIN32
// On MinGW, pthread.h pulls in Windows.h, which in turn pollutes the global
// namespace. We define VC_EXTRALEAN and WIN32_LEAN_AND_MEAN to reduce this.
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif  // _WIN32
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/file/Util.h"
#include "ola/network/IPV4Address.h"
#include "ola/stl/STLUtils.h"
#include "ola/thread/Thread.h"

namespace ola {

using ola::thread::Mutex;
using ola::thread::ConditionVariable;
using std::ifstream;
using std::ofstream;
using std::map;
using std::pair;
using std::string;
using std::vector;

namespace {
void SavePreferencesToFile(
    const string *filename_ptr,
    const FilePreferenceSaverThread::PreferencesMap *pref_map_ptr) {
  std::auto_ptr<const string> filename(filename_ptr);
  std::auto_ptr<const FilePreferenceSaverThread::PreferencesMap> pref_map(
      pref_map_ptr);

  FilePreferenceSaverThread::PreferencesMap::const_iterator iter;
  ofstream pref_file(filename->data());

  if (!pref_file.is_open()) {
    OLA_WARN << "Could not open " << *filename_ptr << ": " << strerror(errno);
    return;
  }

  for (iter = pref_map->begin(); iter != pref_map->end(); ++iter) {
    pref_file << iter->first << " = " << iter->second << std::endl;
  }
  pref_file.flush();
  pref_file.close();
}
}  // namespace

const char BoolValidator::ENABLED[] = "true";
const char BoolValidator::DISABLED[] = "false";

const char FileBackedPreferences::OLA_CONFIG_PREFIX[] = "ola-";
const char FileBackedPreferences::OLA_CONFIG_SUFFIX[] = ".conf";

// Validators
//-----------------------------------------------------------------------------

bool StringValidator::IsValid(const string &value) const {
  return m_empty_ok || !value.empty();
}


bool BoolValidator::IsValid(const string &value) const {
  return (value == ENABLED || value == DISABLED);
}


bool UIntValidator::IsValid(const string &value) const {
  unsigned int output;
  if (!StringToInt(value, &output)) {
    return false;
  }

  return (output >= m_gt && output <= m_lt);
}


bool IntValidator::IsValid(const string &value) const {
  int output;
  if (!StringToInt(value, &output)) {
    return false;
  }

  return (output >= m_gt && output <= m_lt);
}


template <>
bool SetValidator<string>::IsValid(const string &value) const {
  return STLContains(m_values, value);
}


template <>
bool SetValidator<unsigned int>::IsValid(const string &value) const {
  unsigned int output;
  // It's an integer based set validator, so if we can't parse it to an
  // integer, it can't possibly match an integer and be valid
  if (!StringToInt(value, &output)) {
    return false;
  }

  return STLContains(m_values, output);
}


template <>
bool SetValidator<int>::IsValid(const string &value) const {
  int output;
  // It's an integer based set validator, so if we can't parse it to an
  // integer, it can't possibly match an integer and be valid
  if (!StringToInt(value, &output)) {
    return false;
  }

  return STLContains(m_values, output);
}


bool IPv4Validator::IsValid(const string &value) const {
  if (value.empty()) {
    return m_empty_ok;
  }

  vector<string> tokens;
  StringSplit(value, &tokens, ".");
  if (tokens.size() != ola::network::IPV4Address::LENGTH) {
    return false;
  }

  for (unsigned int i = 0 ; i < 4; i++) {
    unsigned int octet;
    if (!StringToInt(tokens[i], &octet)) {
      return false;
    }
    if (octet > UINT8_MAX) {
      return false;
    }
  }
  return true;
}


// Prefs Factory
//-----------------------------------------------------------------------------

PreferencesFactory::~PreferencesFactory() {
  map<string, Preferences*>::const_iterator iter;
  for (iter = m_preferences_map.begin(); iter != m_preferences_map.end();
       ++iter) {
    delete iter->second;
  }
  m_preferences_map.clear();
}


Preferences *PreferencesFactory::NewPreference(const string &name) {
  map<string, Preferences*>::iterator iter = m_preferences_map.find(name);
  if (iter == m_preferences_map.end()) {
    Preferences *pref = Create(name);
    m_preferences_map.insert(make_pair(name, pref));
    return pref;
  } else {
    return iter->second;
  }
}


// Memory Preferences
//-----------------------------------------------------------------------------

MemoryPreferences::~MemoryPreferences() {
  m_pref_map.clear();
}


void MemoryPreferences::Clear() {
  m_pref_map.clear();
}


void MemoryPreferences::SetValue(const string &key,
                                 const string &value) {
  m_pref_map.erase(key);
  m_pref_map.insert(make_pair(key, value));
}


void MemoryPreferences::SetValue(const string &key, unsigned int value) {
  SetValue(key, IntToString(value));
}


void MemoryPreferences::SetValue(const string &key, int value) {
  SetValue(key, IntToString(value));
}


void MemoryPreferences::SetMultipleValue(const string &key,
                                         const string &value) {
  m_pref_map.insert(make_pair(key, value));
}


void MemoryPreferences::SetMultipleValue(const string &key,
                                         unsigned int value) {
  SetMultipleValue(key, IntToString(value));
}


void MemoryPreferences::SetMultipleValue(const string &key, int value) {
  SetMultipleValue(key, IntToString(value));
}


bool MemoryPreferences::SetDefaultValue(const string &key,
                                        const Validator &validator,
                                        const string &value) {
  PreferencesMap::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter == m_pref_map.end() || !validator.IsValid(iter->second)) {
    SetValue(key, value);
    return true;
  }
  return false;
}


bool MemoryPreferences::SetDefaultValue(const string &key,
                                        const Validator &validator,
                                        const char value[]) {
  return SetDefaultValue(key, validator, string(value));
}

bool MemoryPreferences::SetDefaultValue(const string &key,
                                        const Validator &validator,
                                        unsigned int value) {
  return SetDefaultValue(key, validator, IntToString(value));
}


bool MemoryPreferences::SetDefaultValue(const string &key,
                                        const Validator &validator,
                                        int value) {
  return SetDefaultValue(key, validator, IntToString(value));
}


bool MemoryPreferences::SetDefaultValue(const string &key,
                                        const Validator &validator,
                                        const bool value) {
  return SetDefaultValue(
      key,
      validator,
      value ? BoolValidator::ENABLED : BoolValidator::DISABLED);
}


string MemoryPreferences::GetValue(const string &key) const {
  PreferencesMap::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter != m_pref_map.end())
    return iter->second;
  return "";
}


vector<string> MemoryPreferences::GetMultipleValue(const string &key) const {
  vector<string> values;
  PreferencesMap::const_iterator iter;

  for (iter = m_pref_map.find(key);
       iter != m_pref_map.end() && iter->first == key; ++iter) {
    values.push_back(iter->second);
  }
  return values;
}


bool MemoryPreferences::HasKey(const string &key) const {
  return STLContains(m_pref_map, key);
}


void MemoryPreferences::RemoveValue(const string &key) {
  m_pref_map.erase(key);
}


bool MemoryPreferences::GetValueAsBool(const string &key) const {
  PreferencesMap::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter != m_pref_map.end())
    return iter->second == BoolValidator::ENABLED;
  return false;
}


void MemoryPreferences::SetValueAsBool(const string &key, bool value) {
  m_pref_map.erase(key);
  m_pref_map.insert(make_pair(key, (value ? BoolValidator::ENABLED :
                                            BoolValidator::DISABLED)));
}




// FilePreferenceSaverThread
//-----------------------------------------------------------------------------

FilePreferenceSaverThread::FilePreferenceSaverThread()
    : Thread(Thread::Options("pref-saver")) {
  // set a long poll interval so we don't spin
  m_ss.SetDefaultInterval(TimeInterval(60, 0));
}

void FilePreferenceSaverThread::SavePreferences(
    const string &file_name,
    const PreferencesMap &preferences) {
  const string *file_name_ptr = new string(file_name);
  const PreferencesMap *save_map = new PreferencesMap(preferences);
  SingleUseCallback0<void> *cb =
      NewSingleCallback(SavePreferencesToFile, file_name_ptr, save_map);
  m_ss.Execute(cb);
}


void *FilePreferenceSaverThread::Run() {
  m_ss.Run();
  return NULL;
}


bool FilePreferenceSaverThread::Join(void *ptr) {
  m_ss.Terminate();
  return Thread::Join(ptr);
}


void FilePreferenceSaverThread::Synchronize() {
  Mutex synchronize_mutex;
  ConditionVariable condition_var;
  synchronize_mutex.Lock();
  m_ss.Execute(NewSingleCallback(
        this,
        &FilePreferenceSaverThread::CompleteSynchronization,
        &condition_var,
        &synchronize_mutex));
  condition_var.Wait(&synchronize_mutex);
}


void FilePreferenceSaverThread::CompleteSynchronization(
    ConditionVariable *condition,
    Mutex *mutex) {
  // calling lock here forces us to block until Wait() is called on the
  // condition_var.
  mutex->Lock();
  mutex->Unlock();
  condition->Signal();
}


// FileBackedPreferences
//-----------------------------------------------------------------------------

bool FileBackedPreferences::Load() {
  return LoadFromFile(FileName());
}


bool FileBackedPreferences::Save() const {
  m_saver_thread->SavePreferences(FileName(), m_pref_map);
  return true;
}


const string FileBackedPreferences::FileName() const {
  return (m_directory + ola::file::PATH_SEPARATOR + OLA_CONFIG_PREFIX +
          m_preference_name + OLA_CONFIG_SUFFIX);
}


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

    if (line.empty() || line.at(0) == '#') {
      continue;
    }

    vector<string> tokens;
    StringSplit(line, &tokens, "=");

    if (tokens.size() != 2) {
      OLA_INFO << "Skipping line: " << line;
      continue;
    }

    string key = tokens[0];
    string value = tokens[1];
    StringTrim(&key);
    StringTrim(&value);
    m_pref_map.insert(make_pair(key, value));
  }
  pref_file.close();
  return true;
}
}  // namespace ola
