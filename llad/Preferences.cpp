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

#include <lla/Logging.h>
#include <llad/Preferences.h>

#define LLA_CONFIG_DIR ".lla"
#define LLA_CONFIG_PREFIX "lla-"
#define LLA_CONFIG_SUFFIX ".conf"

namespace lla {

/*
 * Destroy this object
 */
MemoryPreferences::~MemoryPreferences() {
  m_pref_map.clear();
}



/*
 * Set a preference value, overiding the existing value.
 *
 * @param key
 * @param value
 */
int MemoryPreferences::SetValue(const string &key, const string &val) {
  m_pref_map.erase(key);
  m_pref_map.insert(pair<string,string>(key,val));
  return 0;
}


/*
 * Adds this preference value to the store
 *
 * @param key
 * @param value
 */
int MemoryPreferences::SetMultipleValue(const string &key, const string &val) {
  m_pref_map.insert(pair<string,string>(key,val));
  return 0;
}


/*
 * Get a preference value
 *
 * @param key
 * @return the value corrosponding to key
 *
 */
string MemoryPreferences::GetValue(const string &key) const {
  map<string, string>::const_iterator iter;
  iter = m_pref_map.find(key);

  if (iter != m_pref_map.end()) {
    return iter->second.c_str();
  }

  return "";
}


/*
 * returns all preference values corrosponding to this key
 *
 * the vector must be freed once used.
 */
vector<string> MemoryPreferences::GetMultipleValue(const string &key) const {
  vector<string> values;
  map<string, string>::const_iterator iter;

  for (iter = m_pref_map.find(key); iter != m_pref_map.end() && iter->first == key; ++iter)
    values.push_back(iter->second);
  return values;
}


// FileBackedPreferences
//-----------------------------------------------------------------------------

/*
 * Load the preferences from storage
 */
int FileBackedPreferences::Load() {
  #define BUF_SIZE 1024
  FILE *fh;
  string filename, line;
  char buf[BUF_SIZE], *k_c, *v_c;
  string key, val;

  if(ChangeDir())
    return -1;

  filename = LLA_CONFIG_PREFIX + m_preference_name + LLA_CONFIG_SUFFIX;
  if ((fh = fopen(filename.c_str(), "r")) == NULL) {
    LLA_INFO << "Missing " << filename << ": " << strerror(errno);
    return -1;
  }

  while (fgets(buf, 1024, fh) != NULL) {
    if (*buf == '#')
      continue;

    k_c = strtok(buf, "=");
    v_c = strtok(NULL, "=");

    if (k_c == NULL || v_c == NULL)
      continue;

    key = StrTrim(k_c);
    val = StrTrim(v_c);

    m_pref_map.insert(pair<string,string>(key,val));
  }

  fclose(fh);
  return 0;
}


/*
 * Save the preferences to storage
 *
 *
 */
int FileBackedPreferences::Save() const {
  map<string, string>::const_iterator iter;
  string filename;
  FILE *fh;

  if (ChangeDir())
    return -1;

  filename = LLA_CONFIG_PREFIX + m_preference_name + LLA_CONFIG_SUFFIX;
  if ((fh = fopen(filename.c_str(), "w")) == NULL) {
    perror("fopen");
    return -1;
  }

  for (iter=m_pref_map.begin(); iter != m_pref_map.end(); ++iter) {
    fprintf(fh, "%s = %s\n", iter->first.c_str() , iter->second.c_str());
  }

  fclose(fh);
  return 0;
}


// Private functions follow
//-----------------------------------------------------------------------------


int FileBackedPreferences::ChangeDir() const {
  struct passwd *ptr = getpwuid(getuid());
  if (ptr == NULL)
    return -1;

  if (chdir(ptr->pw_dir))
    return -1;

  if (chdir(LLA_CONFIG_DIR)) {
    // try and create it
    if (mkdir(LLA_CONFIG_DIR, 0777))
      return -1;

    if (chdir(LLA_CONFIG_DIR))
      return -1;
  }
  return 0;
}


/*
 * trim leading and trailing whitespace from the string
 *
 * @param str  pointer to the string
 */
char *FileBackedPreferences::StrTrim(char *str) {
  int n = strlen(str);
  char *beg, *end;
  beg = str;
  end = str + (n-1); /* Point to the last non-null character in the string */

  while (*beg == ' ' || *beg == '\t')
    beg++;

  /* Remove trailing whitespace and null-terminate */
  while (*end == ' ' || *end == '\n' || *end == '\r' || *beg == '\t' )
    end--;

  *(end+1) = '\0';
  /* Shift the string */
  memmove(str, beg, (end - beg + 2));
  return str;
}

} //lla
