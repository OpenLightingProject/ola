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

#include <ola/Logging.h>
#include <ola/OlaThread.h>
#include <ola/network/SelectServer.h>

#include <map>
#include <vector>
#include <set>
#include <string>


namespace ola {

using std::map;
using std::multimap;
using std::set;
using std::string;
using std::vector;

/*
 * Checks the value of a variable
 */
class Validator {
  public:
    Validator() {}
    virtual ~Validator() {}

    virtual bool IsValid(const string &value) const = 0;
};


/*
 * Check a value is a non-empty string
 */
class StringValidator: public Validator {
  public:
    StringValidator(): Validator() {}
    bool IsValid(const string &value) const;
};


/*
 * Check that a value is one of a set of values
 */
class SetValidator: public Validator {
  public:
    explicit SetValidator(const set<string> &values):
      m_values(values) {}
    bool IsValid(const string &value) const;

  private:
    set<string> m_values;
};


/*
 * Check that a value is a valid bool
 */
class BoolValidator: public Validator {
  public:
    BoolValidator(): Validator() {}
    bool IsValid(const string &value) const;

    // On win32 TRUE and FALSE are #define'd. We can #undef them here but that
    // doesn't fix the case in the calling code. So we use ENABLED and DISABLED
    // instead.
    static const char ENABLED[];
    static const char DISABLED[];
};


/*
 * Check that a value falls within a range
 */
class IntValidator: public Validator {
  public:
    IntValidator(unsigned int greater_than, unsigned int less_than):
        m_gt(greater_than),
        m_lt(less_than) {}
    bool IsValid(const string &value) const;

  private:
    unsigned int m_gt, m_lt;
};


/*
 * Check a IPv4 address is valid
 */
class IPv4Validator: public Validator {
  public:
    explicit IPv4Validator(bool empty_ok = true):
      m_empty_ok(empty_ok) {}

    bool IsValid(const string &value) const;
  private:
    bool m_empty_ok;
};


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
    virtual void SetMultipleValue(const string &key, const string &value) = 0;
    virtual bool SetDefaultValue(const string &key,
                                 const Validator &validator,
                                 const string &value) = 0;

    virtual string GetValue(const string &key) const = 0;
    virtual vector<string> GetMultipleValue(const string &key) const = 0;

    virtual void RemoveValue(const string &key) = 0;

    // bool helper methods
    virtual bool GetValueAsBool(const string &key) const = 0;
    virtual void SetValueAsBool(const string &key, bool value) = 0;

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
    virtual void SetMultipleValue(const string &key, const string &value);
    virtual bool SetDefaultValue(const string &key,
                                 const Validator &validator,
                                 const string &value);

    virtual string GetValue(const string &key) const;
    virtual vector<string> GetMultipleValue(const string &key) const;

    virtual void RemoveValue(const string &key);

    // bool helper methods
    virtual bool GetValueAsBool(const string &key) const;
    virtual void SetValueAsBool(const string &key, bool value);

    bool operator==(const MemoryPreferences &other) {
      return m_pref_map == other.m_pref_map;
    }

  protected:
    typedef multimap<string, string> PreferencesMap;
    PreferencesMap m_pref_map;
};


class MemoryPreferencesFactory: public PreferencesFactory {
  private:
    MemoryPreferences *Create(const string &name) {
      return new MemoryPreferences(name);
    }
};


/**
 * The thread that saves preferences
 */
class FilePreferenceSaverThread: public OlaThread {
  public:
    typedef multimap<string, string> PreferencesMap;
    FilePreferenceSaverThread();

    void SavePreferences(const string &filename,
                         const PreferencesMap &preferences);

    void *Run();
    bool Join(void *ptr = NULL);
    void Syncronize();

  private:
    ola::network::SelectServer m_ss;

    void SaveToFile(const string *filename, const PreferencesMap *preferences);
    void CompleteSyncronization(ConditionVariable *condition);
};


/*
 * FilePreferences uses one file per namespace
 */
class FileBackedPreferences: public MemoryPreferences {
  public:
    explicit FileBackedPreferences(const string &directory,
                                   const string &name,
                                   FilePreferenceSaverThread *saver_thread)
        : MemoryPreferences(name),
          m_directory(directory),
          m_saver_thread(saver_thread) {}
    virtual bool Load();
    virtual bool Save() const;
    bool LoadFromFile(const string &filename);

  private:
    const string m_directory;
    FilePreferenceSaverThread *m_saver_thread;

    bool ChangeDir() const;
    const string FileName() const;
    static const char OLA_CONFIG_PREFIX[];
    static const char OLA_CONFIG_SUFFIX[];
};


class FileBackedPreferencesFactory: public PreferencesFactory {
  public:
    explicit FileBackedPreferencesFactory(const string &directory)
        : m_directory(directory) {
      m_saver_thread.Start();
    }

    ~FileBackedPreferencesFactory() {
      m_saver_thread.Join();
    }

  private:
    const string m_directory;
    FilePreferenceSaverThread m_saver_thread;

    FileBackedPreferences *Create(const string &name) {
      return new FileBackedPreferences(m_directory, name, &m_saver_thread);
    }
};
}  // ola
#endif  // INCLUDE_OLAD_PREFERENCES_H_
