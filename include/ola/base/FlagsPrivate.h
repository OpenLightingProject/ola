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
#ifndef INCLUDE_OLA_BASE_FLAGSPRIVATE_H_
#define INCLUDE_OLA_BASE_FLAGSPRIVATE_H_

#include <getopt.h>
#include <ola/StringUtils.h>
#include <map>
#include <sstream>
#include <string>

namespace ola {

using std::string;

/**
 * The interface for the Flag clases.
 */
class FlagInterface {
  public:
    virtual ~FlagInterface() {}

    virtual const char* name() const = 0;
    virtual char short_opt() const = 0;
    virtual bool has_arg() const = 0;
    virtual const char* arg_type() const = 0;
    virtual string help() const = 0;
    virtual bool SetValue(const string &input) = 0;
};

/**
 * The common implementation.
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
    string help() const { return m_help; }

  protected:
    void ReplaceUnderscoreWithHyphen(char *input);
    const char* NewCanonicalName(const char *name);

  private:
    const char *m_arg_type;
    char m_short_opt;
    const char *m_help;
};

/**
 * A templated Flag class, this one is used for the int types.
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

    bool SetValue(const string &input);

  private:
    const char *m_name;
    T m_default;
    T m_value;
};

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
        unsigned int total_size = strlen(NO_PREFIX) + strlen(name) + 1;
        char* new_name = new char[total_size];
        strncpy(new_name, NO_PREFIX, strlen(NO_PREFIX));
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

    bool SetValue(const string&) {
      m_value = !m_default;
      return true;
    }

  private:
    const char *m_name;
    bool m_default;
    bool m_value;

    static const char NO_PREFIX[];
};


template<>
class Flag<string> : public BaseFlag {
  public:
    Flag(const char *name, const char *arg_type, const char *short_opt,
         string default_value, const char *help)
      : BaseFlag(arg_type, short_opt, help),
        m_name(name),
        m_default(default_value),
        m_value(default_value) {
      m_name = NewCanonicalName(name);
    }

    const char *name() const { return m_name; }
    bool has_arg() const { return true; }
    string default_value() const { return m_default; }

    operator const char*() const { return m_value.c_str(); }

    Flag &operator=(const string &v) {
      m_value = v;
      return *this;
    }

    bool SetValue(const string &input) {
      m_value = input;
      return true;
    }

  private:
    const char *m_name;
    string m_default;
    string m_value;
};


template <typename T>
bool Flag<T>::SetValue(const std::string &input) {
  return ola::StringToInt(input, &m_value, true);
}


/**
 * This class holds all the flags, and is responsbile for parsing the command
 * line.
 */
class FlagRegistry {
  public:
    FlagRegistry() {}

    void RegisterFlag(FlagInterface *flag);
    void ParseFlags(int *argc, char **argv);

    void SetFirstLine(const string &help);
    void SetDecription(const string &help);
    void DisplayUsage();

  private:
    typedef std::map<std::string, FlagInterface*> LongOpts;
    typedef std::map<char, FlagInterface*> ShortOpts;
    typedef std::map<int, FlagInterface*> FlagMap;

    LongOpts m_long_opts;
    ShortOpts m_short_opts;
    string m_argv0;
    string m_first_line;
    string m_description;

    string GetShortOptsString() const;
    struct option *GetLongOpts(FlagMap *flag_map);
};

/**
 * Get the global FlagRegistry.
 */
FlagRegistry *GetRegistry();

/**
 * This class is responsible for registering a flag
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
}  // ola

#define DECLARE_flag(type, name) \
  namespace ola_flags { extern Flag<type> FLAGS_##name  } \
  using ola_flags::FLAGS_##name;

#define DEFINE_flag(type, name, short_opt, default_value, help_str) \
  namespace ola_flags { \
    ola::Flag<type> FLAGS_##name(#name, #type, #short_opt, default_value, \
                                 help_str); \
    ola::FlagRegisterer flag_registerer_##name(&FLAGS_##name); \
  } \
  using ola_flags::FLAGS_##name

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
