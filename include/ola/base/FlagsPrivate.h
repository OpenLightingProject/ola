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
 * FlagsPrivate.h
 * Command line flag (option) handling.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup flags
 * @{
 * @file FlagsPrivate.h
 */

#ifndef INCLUDE_OLA_BASE_FLAGSPRIVATE_H_
#define INCLUDE_OLA_BASE_FLAGSPRIVATE_H_

#include <getopt.h>
#include <string.h>
#include <ola/StringUtils.h>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ola {

/**
 * @brief The interface for the Flag classes.
 */
class FlagInterface {
 public:
    virtual ~FlagInterface() {}

    virtual const char* name() const = 0;
    virtual char short_opt() const = 0;
    virtual bool has_arg() const = 0;
    virtual const char* arg_type() const = 0;
    virtual std::string help() const = 0;
    virtual bool SetValue(const std::string &input) = 0;
};

/**
 * @brief The common implementation.
 */
class BaseFlag : public FlagInterface {
 public:
    BaseFlag(const char *arg_type, const char *short_opt, const char *help)
        : m_arg_type(arg_type),
          m_short_opt(short_opt[0]),
          m_help(help) {
    }

    char short_opt() const { return m_short_opt; }
    const char* arg_type() const { return m_arg_type; }
    std::string help() const { return m_help; }

 protected:
    void ReplaceUnderscoreWithHyphen(char *input);
    const char* NewCanonicalName(const char *name);

 private:
    const char *m_arg_type;
    char m_short_opt;
    const char *m_help;
};

/**
 * @brief A templated Flag class.
 * @tparam T the type of the flag.
 */
template <typename T>
class Flag : public BaseFlag {
 public:
    Flag(const char *name, const char *arg_type, const char *short_opt,
         T default_value, const char *help)
      : BaseFlag(arg_type, short_opt, help),
        m_name(name),
        m_default(default_value),
        m_value(default_value) {
      m_name = NewCanonicalName(name);
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
};

/**
 * @brief a bool flag
 */
template<>
class Flag<bool> : public BaseFlag {
 public:
    Flag(const char *name, const char *arg_type, const char *short_opt,
         bool default_value, const char *help)
      : BaseFlag(arg_type, short_opt, help),
        m_name(name),
        m_default(default_value),
        m_value(default_value) {
      if (default_value) {
        // prefix the long option with 'no'
        size_t total_size = strlen(NO_PREFIX) + strlen(name) + 1;
        char* new_name = new char[total_size];
        strncpy(new_name, NO_PREFIX, strlen(NO_PREFIX) + 1);
        strncat(new_name, name, total_size);
        ReplaceUnderscoreWithHyphen(new_name);
        m_name = new_name;
      } else {
        m_name = NewCanonicalName(name);
      }
    }

    const char *name() const { return m_name; }
    bool has_arg() const { return false; }
    bool default_value() const { return m_default; }

    operator bool() const { return m_value; }

    Flag &operator=(bool v) {
      m_value = v;
      return *this;
    }

    bool SetValue(const std::string&) {
      m_value = !m_default;
      return true;
    }

 private:
    const char *m_name;
    bool m_default;
    bool m_value;

    static const char NO_PREFIX[];
};

/**
 * @brief a string flag
 */
template<>
class Flag<std::string> : public BaseFlag {
 public:
    Flag(const char *name, const char *arg_type, const char *short_opt,
         std::string default_value, const char *help)
      : BaseFlag(arg_type, short_opt, help),
        m_name(name),
        m_default(default_value),
        m_value(default_value) {
      m_name = NewCanonicalName(name);
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
      m_value = input;
      return true;
    }

 private:
    const char *m_name;
    std::string m_default;
    std::string m_value;
};

/**
 * @brief Used to set the value of a flag
 */
template <typename T>
bool Flag<T>::SetValue(const std::string &input) {
  return ola::StringToInt(input, &m_value, true);
}


/**
 * @brief This class holds all the flags, and is responsbile for parsing the
 * command line.
 */
class FlagRegistry {
 public:
    FlagRegistry() {}

    void RegisterFlag(FlagInterface *flag);
    void ParseFlags(int *argc, char **argv);

    void SetFirstLine(const std::string &help);
    void SetDecription(const std::string &help);
    void DisplayUsage();
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
};
}  // namespace ola

/**
 * @brief Declare a flag which was defined in another file.
 */
#define DECLARE_flag(type, name) \
  namespace ola_flags { extern ola::Flag<type> FLAGS_##name; } \
  using ola_flags::FLAGS_##name;

/**
 * @brief Generic macro to define a flag
 */
#define DEFINE_flag(type, name, short_opt, default_value, help_str) \
  namespace ola_flags { \
    ola::Flag<type> FLAGS_##name(#name, #type, #short_opt, default_value, \
                                 help_str); \
    ola::FlagRegisterer flag_registerer_##name(&FLAGS_##name); \
  } \
  using ola_flags::FLAGS_##name

/**
 * @brief Generic macro to define a flag with a short option.
 */
#define DEFINE_flag_with_short(type, name, short_opt, default_value, help_str) \
  namespace ola_flags { char flag_short_##short_opt = 0; } \
  namespace ola_flags { \
    ola::Flag<type> FLAGS_##name(#name, #type, #short_opt, default_value, \
                                 help_str); \
    ola::FlagRegisterer flag_registerer_##name( \
        &FLAGS_##name, &flag_short_##short_opt); \
  } \
  using ola_flags::FLAGS_##name

#endif  // INCLUDE_OLA_BASE_FLAGSPRIVATE_H_
/**@}*/
