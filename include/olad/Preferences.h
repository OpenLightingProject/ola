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
 * Preferences.h
 * Interface for the Preferences class - this allows storing user preferences /
 * settings.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef INCLUDE_OLAD_PREFERENCES_H_
#define INCLUDE_OLAD_PREFERENCES_H_

#include <ola/base/Macro.h>
#include <ola/Logging.h>
#include <ola/io/SelectServer.h>
// On MinGW, Thread.h pulls in pthread.h which pulls in Windows.h, which needs
// to be after WinSock2.h, hence this order
#include <ola/thread/Thread.h>

#include <map>
#include <vector>
#include <set>
#include <string>


namespace ola {

/*
 * Checks the value of a variable
 */
class Validator {
 public:
  Validator() {}
  virtual ~Validator() {}

  virtual bool IsValid(const std::string &value) const = 0;
};


/*
 * Check a value is a non-empty string
 */
class StringValidator: public Validator {
 public:
  explicit StringValidator(bool empty_ok = false)
      : Validator(),
        m_empty_ok(empty_ok) {
  }
  bool IsValid(const std::string &value) const;

 private:
    const bool m_empty_ok;
};


/*
 * Check that a value is one of a set of values
 */
template <class T>
class SetValidator: public Validator {
 public:
  explicit SetValidator(const std::set<T> &values) : m_values(values) {}
  bool IsValid(const std::string &value) const;

 private:
  std::set<T> m_values;
};


/*
 * Check that a value is a valid bool
 */
class BoolValidator: public Validator {
 public:
  BoolValidator(): Validator() {}
  bool IsValid(const std::string &value) const;

  // On win32 TRUE and FALSE are #define'd. We can #undef them here but that
  // doesn't fix the case in the calling code. So we use ENABLED and DISABLED
  // instead.
  static const char ENABLED[];
  static const char DISABLED[];
};


/*
 * Check that a value falls within a range of unsigned ints.
 */
class UIntValidator: public Validator {
 public:
  UIntValidator(unsigned int greater_than, unsigned int less_than)
      : m_gt(greater_than),
        m_lt(less_than) {}
  bool IsValid(const std::string &value) const;

 private:
  unsigned int m_gt, m_lt;
};


/*
 * Check that a value falls within a range of ints.
 */
class IntValidator: public Validator {
 public:
  IntValidator(int greater_than, int less_than)
      : m_gt(greater_than),
        m_lt(less_than) {}
  bool IsValid(const std::string &value) const;

 private:
  int m_gt, m_lt;
};


/*
 * Check an IPv4 address is valid
 */
class IPv4Validator: public Validator {
 public:
  explicit IPv4Validator(bool empty_ok = true):
    m_empty_ok(empty_ok) {}

  bool IsValid(const std::string &value) const;
 private:
  bool m_empty_ok;

  DISALLOW_COPY_AND_ASSIGN(IPv4Validator);
};


/*
 * The abstract Preferences class
 */
class Preferences {
 public:
  explicit Preferences(const std::string name): m_preference_name(name) {}

  /**
   * Destroy this object
   */
  virtual ~Preferences() {}

  /**
   * Load the preferences from storage
   */
  virtual bool Load() = 0;

  /**
   * Save the preferences to storage
   */
  virtual bool Save() const = 0;

  /**
   * Clear the preferences
   */
  virtual void Clear() = 0;

  /**
   * @brief The location of where these preferences are stored.
   * @return the location
   */
  virtual std::string ConfigLocation() const = 0;

  /**
   * @brief Set a preference value, overriding the existing value.
   * @param key
   * @param value
   */
  virtual void SetValue(const std::string &key, const std::string &value) = 0;

  /**
   * @brief Set a preference value, overriding the existing value. This helper
   * accepts an unsigned int.
   * @param key
   * @param value
   */
  virtual void SetValue(const std::string &key, unsigned int value) = 0;

  /**
   * @brief Set a preference value, overriding the existing value. This helper
   * accepts an int.
   * @param key
   * @param value
   */
  virtual void SetValue(const std::string &key, int value) = 0;

  /**
   * @brief Adds this preference value to the store.
   * @param key
   * @param value
   */
  virtual void SetMultipleValue(const std::string &key,
                                const std::string &value) = 0;

  /**
   * @brief Adds this preference value to the store. This helper accepts an
   * unsigned int.
   * @param key
   * @param value
   */
  virtual void SetMultipleValue(const std::string &key, unsigned int value) = 0;

  /**
   * @brief Adds this preference value to the store. This helper accepts an
   * int.
   * @param key
   * @param value
   */
  virtual void SetMultipleValue(const std::string &key, int value) = 0;

  /**
   * @brief Set a preference value if it doesn't already exist, or if it exists
   * and doesn't pass the validator.
   * @note Note this only checks the first value's validity.
   * @param key the key to check/set
   * @param validator A Validator object
   * @param value the new value
   * @return true if we set the value, false if it already existed
   */
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               const std::string &value) = 0;

  /**
   * @brief Set a preference value if it doesn't already exist, or if it exists
   * and doesn't pass the validator. This helper accepts a char array to force
   * it to a string.
   * @note Note this only checks the first value's validity.
   * @param key the key to check/set
   * @param validator A Validator object
   * @param value the new value
   * @return true if we set the value, false if it already existed
   */
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               const char value[]) = 0;

  /**
   * @brief Set a preference value if it doesn't already exist, or if it exists
   * and doesn't pass the validator. This helper accepts an unsigned int value
   * @note Note this only checks the first value's validity.
   * @param key the key to check/set
   * @param validator A Validator object
   * @param value the new value
   * @return true if we set the value, false if it already existed
   */
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               unsigned int value) = 0;

  /**
   * @brief Set a preference value if it doesn't already exist, or if it exists
   * and doesn't pass the validator. This helper accepts an int value
   * @note Note this only checks the first value's validity.
   * @param key the key to check/set
   * @param validator A Validator object
   * @param value the new value
   * @return true if we set the value, false if it already existed
   */
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               int value) = 0;

  /**
   * @brief Set a preference value if it doesn't already exist, or if it exists
   * and doesn't pass the validator. This helper accepts a bool value
   * @note Note this only checks the first value's validity.
   * @param key the key to check/set
   * @param validator A Validator object
   * @param value the new value
   * @return true if we set the value, false if it already existed
   */
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               bool value) = 0;

  /**
   * @brief Get a preference value
   * @param key the key to fetch
   * @return the value corresponding to key, or the empty string if the key
   * doesn't exist.
   */
  virtual std::string GetValue(const std::string &key) const = 0;

  /**
   * @brief Returns all preference values corresponding to this key
   * @param key the key to fetch
   * @return a vector of strings.
   */
  virtual std::vector<std::string> GetMultipleValue(
      const std::string &key) const = 0;

  /**
   * @brief Check if a preference key exists
   * @param key the key to check
   * @return true if the key exists, false otherwise.
   */
  virtual bool HasKey(const std::string &key) const = 0;

  /**
   * @brief Remove a preference value.
   * @param key
   */
  virtual void RemoveValue(const std::string &key) = 0;

  // bool helper methods
  /**
   * @brief Get a preference value as a bool
   * @param key the key to fetch
   * @return true if the value is 'true' or false otherwise
   */
  virtual bool GetValueAsBool(const std::string &key) const = 0;

  /**
   * @brief Set a value as a bool.
   * @param key
   * @param value
   */
  virtual void SetValueAsBool(const std::string &key, bool value) = 0;

 protected:
  std::string m_preference_name;

 private:
  DISALLOW_COPY_AND_ASSIGN(Preferences);
};


/**
 * A PreferencesFactory creates preferences objects
 */
class PreferencesFactory {
 public:
  PreferencesFactory() {}

  /**
   * Cleanup
   */
  virtual ~PreferencesFactory();

  /**
   * Lookup a preference object
   */
  virtual Preferences *NewPreference(const std::string &name);

  /**
   * @brief The location where preferences will be stored.
   * @return the location
   */
  virtual std::string ConfigLocation() const = 0;

 private:
  virtual Preferences *Create(const std::string &name) = 0;
  std::map<std::string, Preferences*> m_preferences_map;
};


/*
 * MemoryPreferences just stores the preferences in memory. Useful for testing.
 */
class MemoryPreferences: public Preferences {
 public:
  explicit MemoryPreferences(const std::string name): Preferences(name) {}
  virtual ~MemoryPreferences();
  virtual bool Load() { return true; }
  virtual bool Save() const { return true; }
  virtual void Clear();

  virtual std::string ConfigLocation() const { return "Not Saved"; }

  virtual void SetValue(const std::string &key, const std::string &value);
  virtual void SetValue(const std::string &key, unsigned int value);
  virtual void SetValue(const std::string &key, int value);
  virtual void SetMultipleValue(const std::string &key,
                                const std::string &value);
  virtual void SetMultipleValue(const std::string &key, unsigned int value);
  virtual void SetMultipleValue(const std::string &key, int value);
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               const std::string &value);
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               const char value[]);
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               unsigned int value);
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               int value);
  virtual bool SetDefaultValue(const std::string &key,
                               const Validator &validator,
                               bool value);

  virtual std::string GetValue(const std::string &key) const;
  virtual std::vector<std::string> GetMultipleValue(
      const std::string &key) const;
  virtual bool HasKey(const std::string &key) const;

  virtual void RemoveValue(const std::string &key);

  // bool helper methods
  virtual bool GetValueAsBool(const std::string &key) const;
  virtual void SetValueAsBool(const std::string &key, bool value);

  bool operator==(const MemoryPreferences &other) {
    return m_pref_map == other.m_pref_map;
  }

 protected:
  typedef std::multimap<std::string, std::string> PreferencesMap;
  PreferencesMap m_pref_map;
};


class MemoryPreferencesFactory: public PreferencesFactory {
 public:
  virtual std::string ConfigLocation() const { return "Not Saved"; }

 private:
  MemoryPreferences *Create(const std::string &name) {
    return new MemoryPreferences(name);
  }
};


/**
 * The thread that saves preferences
 */
class FilePreferenceSaverThread: public ola::thread::Thread {
 public:
  typedef std::multimap<std::string, std::string> PreferencesMap;
  FilePreferenceSaverThread();

  void SavePreferences(const std::string &filename,
                       const PreferencesMap &preferences);

  /**
   * Called by the new thread.
   */
  void *Run();

  /**
   * Stop the saving thread
   */
  bool Join(void *ptr = NULL);

  /**
   * This can be used to synchronize with the file saving thread. Useful if you
   * want to make sure the files have been written to disk before continuing.
   * This blocks until all pending save requests are complete.
   */
  void Synchronize();

 private:
  ola::io::SelectServer m_ss;

  /**
   * Notify the blocked thread we're done
   */
  void CompleteSynchronization(ola::thread::ConditionVariable *condition,
                               ola::thread::Mutex *mutex);
};


/*
 * FilePreferences uses one file per namespace
 */
class FileBackedPreferences: public MemoryPreferences {
 public:
  explicit FileBackedPreferences(const std::string &directory,
                                 const std::string &name,
                                 FilePreferenceSaverThread *saver_thread)
      : MemoryPreferences(name),
        m_directory(directory),
        m_saver_thread(saver_thread) {}

  virtual bool Load();
  virtual bool Save() const;

  /**
   * @brief Load these preferences from a file
   * @param filename the filename to load from
   */
  bool LoadFromFile(const std::string &filename);

  std::string ConfigLocation() const { return FileName(); }

 private:
  const std::string m_directory;
  FilePreferenceSaverThread *m_saver_thread;

  bool ChangeDir() const;

  /**
   * Return the name of the file used to save the preferences
   */
  const std::string FileName() const;
  static const char OLA_CONFIG_PREFIX[];
  static const char OLA_CONFIG_SUFFIX[];
};


class FileBackedPreferencesFactory: public PreferencesFactory {
 public:
  explicit FileBackedPreferencesFactory(const std::string &directory)
      : m_directory(directory) {
    m_saver_thread.Start();
  }

  ~FileBackedPreferencesFactory() {
    m_saver_thread.Join();
  }

  virtual std::string ConfigLocation() const { return m_directory; }

 private:
  const std::string m_directory;
  FilePreferenceSaverThread m_saver_thread;

  FileBackedPreferences *Create(const std::string &name) {
    return new FileBackedPreferences(m_directory, name, &m_saver_thread);
  }
};
}  // namespace ola
#endif  // INCLUDE_OLAD_PREFERENCES_H_
