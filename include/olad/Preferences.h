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
 * preferences.h
 * Interface for the Preferences class - this allows storing user preferences /
 * settings.
 * Copyright (C) 2005-2006  Simon Newton
 */

#ifndef INCLUDE_OLAD_PREFERENCES_H_
#define INCLUDE_OLAD_PREFERENCES_H_

#include <map>
#include <vector>
#include <string>


namespace ola {

using std::map;
using std::multimap;
using std::string;
using std::vector;

/*
 * The abstract Preferences class
 */
class Preferences {
  public:
    explicit Preferences(const string name): m_preference_name(name) {}
    virtual ~Preferences() {}

    virtual bool Load() = 0;
    virtual bool Save() const = 0;
    virtual void Clear() = 0;

    virtual void SetValue(const string &key, const string &value) = 0;
    virtual bool SetDefaultValue(const string &key, const string &value) = 0;
    virtual void RemoveValue(const string &key) = 0;
    virtual void SetMultipleValue(const string &key, const string &value) = 0;

    virtual string GetValue(const string &key) const = 0;
    virtual vector<string> GetMultipleValue(const string &key) const = 0;

  protected:
    string m_preference_name;
  private:
    Preferences(const Preferences&);
    Preferences& operator=(const Preferences&);
};


/*
 * A PreferencesFactory creates preferences objects
 */
class PreferencesFactory {
  public:
    PreferencesFactory() {}
    virtual ~PreferencesFactory();
    virtual Preferences *NewPreference(const string &name);
  private:
    virtual Preferences *Create(const string &name) = 0;
    map<string, Preferences*> m_preferences_map;
};


/*
 * MemoryPreferences just stores the preferences in memory. Useful for testing.
 */
class MemoryPreferences: public Preferences {
  public:
    explicit MemoryPreferences(const string name): Preferences(name) {}
    virtual ~MemoryPreferences();
    virtual bool Load() { return true; }
    virtual bool Save() const { return true; }
    virtual void Clear();
    virtual void SetValue(const string &key, const string &value);
    virtual bool SetDefaultValue(const string &key, const string &value);
    virtual void RemoveValue(const string &key);
    virtual void SetMultipleValue(const string &key, const string &value);
    virtual string GetValue(const string &key) const;
    virtual vector<string> GetMultipleValue(const string &key) const;

    bool operator==(const MemoryPreferences &other) {
      return m_pref_map == other.m_pref_map;
    }

  protected:
    multimap<string, string> m_pref_map;
};


class MemoryPreferencesFactory: public PreferencesFactory {
  private:
    MemoryPreferences *Create(const string &name) {
      return new MemoryPreferences(name);
    }
};


/*
 * FilePreferences uses one file per namespace
 */
class FileBackedPreferences: public MemoryPreferences {
  public:
    explicit FileBackedPreferences(const string name)
        : MemoryPreferences(name) {}
    virtual bool Load();
    virtual bool Save() const;
    bool LoadFromFile(const string &filename);
    bool SaveToFile(const string &filename) const;

  private:
    bool ChangeDir() const;
    const string FileName() const;
    static const string OLA_CONFIG_DIR;
    static const string OLA_CONFIG_PREFIX;
    static const string OLA_CONFIG_SUFFIX;
};

class FileBackedPreferencesFactory: public PreferencesFactory {
  private:
    FileBackedPreferences *Create(const string &name) {
      return new FileBackedPreferences(name);
    }
};
}  // ola
#endif  // INCLUDE_OLAD_PREFERENCES_H_
