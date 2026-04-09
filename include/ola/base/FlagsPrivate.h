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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * FlagsPrivate.h
 * Command line flag (option) handling.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup flags
 * @{
 * @file FlagsPrivate.h
 * @brief Internal functionality for the flags.
 * @}
 */

#ifndef INCLUDE_OLA_BASE_FLAGSPRIVATE_H_
#define INCLUDE_OLA_BASE_FLAGSPRIVATE_H_

#include <getopt.h>
#include <string.h>
#include <ola/StringUtils.h>
#include <ola/base/Macro.h>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ola {

/**
 * @addtogroup flags
 * @{
 */

/**
 * @brief The interface for the Flag classes.
 */
class FlagInterface {
 public:
  virtual ~FlagInterface() {}

  /**
   * @brief Get the flag name
   */
  virtual const char* name() const = 0;

  /**
   * @brief Get the flag short option
   */
  virtual char short_opt() const = 0;

  /**
   * @brief Whether the flag requires an argument
   */
  virtual bool has_arg() const = 0;

  /**
   * @brief Get the flag argument type
   */
  virtual const char* arg_type() const = 0;

  /**
   * @brief Get the flag help string
   */
  virtual std::string help() const = 0;

  /**
   * @brief Check if the flag was present on the command line.
   * Good for switching behaviour when a flag is used.
   * @returns true if the flag was present, false otherwise
   */
  virtual bool present() const = 0;

  /**
   * @brief Set the flag value
   * @param input the input passed on the command line
   * @returns true on success, false otherwise
   */
  virtual bool SetValue(const std::string &input) = 0;
};

/**
 * @brief The common implementation.
 */
class BaseFlag : public FlagInterface {
 public:
  /**
   * @brief Create a new BaseFlag
   * @param arg_type the type of flag argument
   * @param short_opt the short option for the flag
   * @param help the help string for the flag
   */
  BaseFlag(const char *arg_type, const char *short_opt, const char *help)
      : m_arg_type(arg_type),
        m_short_opt(short_opt[0]),
        m_help(help),
        m_present(false) {
  }

  char short_opt() const { return m_short_opt; }
  const char* arg_type() const { return m_arg_type; }
  std::string help() const { return m_help; }
  bool present() const { return m_present; }

  /**
   * @brief Set that the flag was present on the command line
   */
  void MarkAsPresent() { m_present = true; }

 protected:
  void ReplaceUnderscoreWithHyphen(char *input);
  const char* NewCanonicalName(const char *name);

 private:
  const char *m_arg_type;
  char m_short_opt;
  const char *m_help;
  bool m_present;
};

/**
 * @brief A templated Flag class.
 * @tparam T the type of the flag.
 */
template <typename T>
class Flag : public BaseFlag {
 public:
  /**
   * @brief Create a new Flag
   * @param name the name of the flag
   * @param arg_type the type of flag argument
   * @param short_opt the short option for the flag
   * @param default_value the flag's default value
   * @param help the help string for the flag
   * @param has_arg if the flag should use an argument, only overrides
   *        Flag<bool>
   */
  Flag(const char *name, const char *arg_type, const char *short_opt,
       T default_value, const char *help,
       OLA_UNUSED const bool has_arg)
    : BaseFlag(arg_type, short_opt, help),
      m_name(name),
      m_default(default_value),
      m_value(default_value) {
    m_name = NewCanonicalName(name);
  }

  ~Flag() {
    delete[] m_name;
  }

  const char *name() const { return m_name; }
  bool has_arg() const { return true; }
  bool default_value() const { return m_default; }

  operator T() const { return m_value; }

  Flag &operator=(T v) {
    m_value = v;
    return *this;
  }

  bool SetValue(const std::string &input);

 private:
  const char *m_name;
  T m_default;
  T m_value;

  DISALLOW_COPY_AND_ASSIGN(Flag);
};

/**
 * @brief a bool flag
 */
template<>
class Flag<bool> : public BaseFlag {
 public:
  Flag(const char *name, const char *arg_type, const char *short_opt,
       bool default_value, const char *help, const bool has_arg)
    : BaseFlag(arg_type, short_opt, help),
      m_name(name),
      m_default(default_value),
      m_value(default_value),
      m_has_arg(has_arg) {
    if (!has_arg && default_value) {
      // prefix the long option with 'no'
      size_t prefix_size = strlen(NO_PREFIX);
      size_t name_size = strlen(name);
      char* new_name = new char[prefix_size + name_size + 1];
      memcpy(new_name, NO_PREFIX, prefix_size);
      memcpy(new_name + prefix_size, name, name_size);
      new_name[prefix_size + name_size] = 0;
      ReplaceUnderscoreWithHyphen(new_name);
      m_name = new_name;
    } else {
      m_name = NewCanonicalName(name);
    }
  }

  ~Flag() {
    delete[] m_name;
  }

  const char *name() const { return m_name; }
  bool has_arg() const { return m_has_arg; }
  bool default_value() const { return m_default; }

  operator bool() const { return m_value; }

  Flag &operator=(bool v) {
    m_value = v;
    return *this;
  }

  bool SetValue(const std::string &input) {
    MarkAsPresent();
    if (m_has_arg) {
      return ola::StringToBoolTolerant(input, &m_value);
    } else {
      m_value = !m_default;
      return true;
    }
  }

 private:
  const char *m_name;
  bool m_default;
  bool m_value;
  bool m_has_arg;

  static const char NO_PREFIX[];

  DISALLOW_COPY_AND_ASSIGN(Flag);
};

/**
 * @brief a string flag
 */
template<>
class Flag<std::string> : public BaseFlag {
 public:
  Flag(const char *name, const char *arg_type, const char *short_opt,
       std::string default_value, const char *help,
       OLA_UNUSED const bool has_arg)
    : BaseFlag(arg_type, short_opt, help),
      m_name(name),
      m_default(default_value),
      m_value(default_value) {
    m_name = NewCanonicalName(name);
  }

  ~Flag() {
    delete[] m_name;
  }

  const char *name() const { return m_name; }
  bool has_arg() const { return true; }
  std::string default_value() const { return m_default; }
  const char* arg_type() const { return "string"; }

  operator const char*() const { return m_value.c_str(); }
  operator std::string() const { return m_value; }
  std::string str() const { return m_value; }

  Flag &operator=(const std::string &v) {
    m_value = v;
    return *this;
  }

  bool SetValue(const std::string &input) {
    MarkAsPresent();
    m_value = input;
    return true;
  }

 private:
  const char *m_name;
  std::string m_default;
  std::string m_value;

  DISALLOW_COPY_AND_ASSIGN(Flag);
};

/**
 * @brief Used to set the value of a flag
 */
template <typename T>
bool Flag<T>::SetValue(const std::string &input) {
  MarkAsPresent();
  return ola::StringToInt(input, &m_value, true);
}


/**
 * @brief This class holds all the flags, and is responsible for parsing the
 * command line.
 */
class FlagRegistry {
 public:
  FlagRegistry() {}

  void RegisterFlag(FlagInterface *flag);
  void ParseFlags(int *argc, char **argv);

  void SetFirstLine(const std::string &help);
  void SetDescription(const std::string &help);
  void DisplayUsage();
  void DisplayVersion();
  void GenManPage();

 private:
  typedef std::map<std::string, FlagInterface*> LongOpts;
  typedef std::map<char, FlagInterface*> ShortOpts;
  typedef std::map<int, FlagInterface*> FlagMap;
  // <flag, description>
  typedef std::pair<std::string, std::string> OptionPair;

  LongOpts m_long_opts;
  ShortOpts m_short_opts;
  std::string m_argv0;
  std::string m_first_line;
  std::string m_description;

  std::string GetShortOptsString() const;
  struct option *GetLongOpts(FlagMap *flag_map);
  void PrintFlags(std::vector<std::string> *lines);
  void PrintManPageFlags(std::vector<OptionPair> *lines);

  DISALLOW_COPY_AND_ASSIGN(FlagRegistry);
};

/**
 * @brief Get the global FlagRegistry.
 */
FlagRegistry *GetRegistry();

/**
 * @brief This class is responsible for registering a flag
 */
class FlagRegisterer {
 public:
  explicit FlagRegisterer(FlagInterface *flag) {
    GetRegistry()->RegisterFlag(flag);
  }

  FlagRegisterer(FlagInterface *flag, char *short_opt) {
    *short_opt = flag->short_opt();
    GetRegistry()->RegisterFlag(flag);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FlagRegisterer);
};

/** @} */

}  // namespace ola

/**
 * @cond HIDDEN_SYMBOLS
 */

/**
 * @brief Declare a flag which was defined in another file.
 */
#define DECLARE_flag(type, name) \
  namespace ola_flags { extern ola::Flag<type> FLAGS_##name; } \
  using ola_flags::FLAGS_##name;

/**
 * @brief Generic macro to define a flag
 */
#define DEFINE_flag(type, name, short_opt, default_value, help_str, \
                    has_arg) \
  namespace ola_flags { \
    ola::Flag<type> FLAGS_##name(#name, #type, #short_opt, default_value, \
                                 help_str, has_arg); \
    ola::FlagRegisterer flag_registerer_##name(&FLAGS_##name); \
  } \
  using ola_flags::FLAGS_##name

/**
 * @brief Generic macro to define a flag with a short option.
 */
#define DEFINE_flag_with_short(type, name, short_opt, default_value, help_str, \
                               has_arg) \
  namespace ola_flags { char flag_short_##short_opt = 0; } \
  namespace ola_flags { \
    ola::Flag<type> FLAGS_##name(#name, #type, #short_opt, default_value, \
                                 help_str, has_arg); \
    ola::FlagRegisterer flag_registerer_##name( \
        &FLAGS_##name, &flag_short_##short_opt); \
  } \
  using ola_flags::FLAGS_##name

/**
 * @endcond
 * End Hidden Symbols
 */

#endif  // INCLUDE_OLA_BASE_FLAGSPRIVATE_H_
