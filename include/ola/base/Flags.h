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
 * Flags.h
 * Command line flag (option) handling.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @defgroup flags Command Line Flags
 * @brief Command line flag processing.
 *
 * This is based on gflags (https://code.google.com/p/gflags/) but we reduce
 * the feature set to make things simpler.
 *
 * Features:
 *  - bool, uint8, uint16, uint32, int8, int16, int32 & string types.
 *  - short options (e.g. -x).
 *  - inverted bools, e.g. \--no-foo
 *
 * @note
 *   - Setting flags is not thread safe
 *   - Flags cannot be used at global construction time.
 *   - DEFINE_ and DECLARE_ must be outside of any namespaces.
 *
 * @examplepara
 *  @snippet flags.cpp Example
 *  When <tt>./flags</tt> is run, this produces:
 *  @code
 *  --foo is 0
 *  --bar is 1
 *  --name is simon
 *  --baz (-b) is 0
 *  @endcode
 *  Compare with <tt>./flags \--foo \--name bob -b 10 \--no-bar</tt>
 *  @code
 *  --foo is 1
 *  --bar is 0
 *  --name is bob
 *  --baz (-b) is 10
 *  @endcode
 *
 * @examplepara - Use flags from other files
 *  You can access flags from files other than the one they were DEFINE_*'d in
 *  by using DECLARE_*:
 *  @code
 *  DECLARE_bool(foo);
 *  DECLARE_bool(bar);
 *  // Can now use FLAGS_foo and FLAGS_bar
 *  @endcode
 */

/**
 * @addtogroup flags
 * @{
 * @file Flags.h
 * @brief Defines macros to ease creation of command line flags
 */
#ifndef INCLUDE_OLA_BASE_FLAGS_H_
#define INCLUDE_OLA_BASE_FLAGS_H_

#include <getopt.h>
#include <ola/base/FlagsPrivate.h>
#include <string>

namespace ola {

/**
 * @brief Set the help string for the program.
 * @param first_line the initial line that is displayed in the help section.
 *     This is displayed after argv[0].
 * @param description a multiline description of the program
 */
void SetHelpString(const std::string &first_line,
                   const std::string &description);

/**
 * @brief Print the usage text to stdout.
 */
void DisplayUsage();

/**
 * @brief Print the usage text to stdout then exit.
 */
void DisplayUsageAndExit();

/**
 * @brief Print the version text to stdout.
 */
void DisplayVersion();

/**
 * @brief Parses the command line flags up to the first non-flag value. argv is
 * re-arranged so that it only contains non-flag arguments.
 *
 * @param argc the argument count taken straight from your main()
 * @param argv the argument vector which holds the actual arguments from the
 * command line. Also comes from main().
 */
void ParseFlags(int *argc, char **argv);
}  // namespace ola

// DECLARE_*

/**
 * @brief Reuse a bool flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_bool(name) \
  DECLARE_flag(bool, name)

/**
 * @brief Reuse an int8_t flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_int8(name) \
  DECLARE_flag(int8_t, name)

/**
 * @brief Reuse an int16_t flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_int16(name) \
  DECLARE_flag(int16_t, name)

/**
 * @brief Reuse an int32_t flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_int32(name) \
  DECLARE_flag(int32_t, name)

/**
 * @brief Reuse a uint8_t flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_uint8(name) \
  DECLARE_flag(uint8_t, name)

/**
 * @brief Reuse a uint16_t flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_uint16(name) \
  DECLARE_flag(uint16_t, name)

/**
 * @brief Reuse a uint32_t flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_uint32(name) \
  DECLARE_flag(uint32_t, name)

/**
 * @brief Reuse a string flag from another file
 * @param name the name of the flag to reuse
 */
#define DECLARE_string(name) \
  DECLARE_flag(std::string, name)

// DEFINE_*

/**
 * @brief Create a new longname bool flag
 *
 * By default the flag is undefined, the same as the string and int ones, that
 * is is_present() returns false. If the flag is provided on the command line,
 * is_present() will be true, and operator bool() returns the value of the
 * flag.
 *
 * @note The value must be parseable by StringToBoolTolerant.
 * @param name the name of the flag to create
 * @param default_value the default value for the flag. Either true, or false.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_bool(name, default_value, help_str) \
  DEFINE_flag(bool, name, \0, default_value, help_str, true)

/**
 * @brief Create a new bool flag with a long and short name
 *
 * By default the flag is undefined, the same as the string and int ones, that
 * is is_present() returns false. If the flag is provided on the command line,
 * is_present() will be true, and operator bool() returns the value of the
 * flag.
 *
 * @note The value must be parseable by StringToBoolTolerant.
 * @param name the name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag. Either true, or false.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_bool(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(bool, name, short_opt, default_value, help_str, true)

/**
 * @brief Create a new longname bool flag that doesn't require an argument.
 *
 * By default the flag is set to default_value. If the flag is provided on the
 * command line, the value of the flag becomes !default_value.
 *
 * @param name the name of the flag to create
 * @param default_value the default value for the flag. Either true, or false.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_default_bool(name, default_value, help_str) \
  DEFINE_flag(bool, name, \0, default_value, help_str, false)

/**
 * @brief Create a new bool flag with a long and short name that doesn't
 *     require an argument.
 *
 * By default the flag is set to default_value. If the flag is provided on the
 * command line, the value of the flag becomes !default_value.
 *
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag. Either true, or false.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_default_bool(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(bool, name, short_opt, default_value, help_str, false)

/**
 * @brief Create a new longname int8_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_int8(name, default_value, help_str) \
  DEFINE_flag(int8_t, name, \0, default_value, help_str, true)

/**
 * @brief Create a new int8_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_int8(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int8_t, name, short_opt, default_value, help_str, \
                         true)

/**
 * @brief Create a new longname uint8_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_uint8(name, default_value, help_str) \
  DEFINE_flag(uint8_t, name, \0, default_value, help_str, true)

/**
 * @brief Create a new uint8_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_uint8(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint8_t, name, short_opt, default_value, help_str, \
                         true)

/**
 * @brief Create a new longname int16_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_int16(name, default_value, help_str) \
  DEFINE_flag(int16_t, name, \0, default_value, help_str, true)

/**
 * @brief Create a new int16_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_int16(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int16_t, name, short_opt, default_value, help_str, \
                         true)


/**
 * @brief Create a new longname uint16_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_uint16(name, default_value, help_str) \
  DEFINE_flag(uint16_t, name, \0, default_value, help_str, true)

/**
 * @brief Create a new uint16_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_uint16(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint16_t, name, short_opt, default_value, help_str, \
                         true)

/**
 * @brief Create a new longname int32_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_int32(name, default_value, help_str) \
  DEFINE_flag(int32_t, name, \0, default_value, help_str, true)

/**
 * @brief Create a new int32_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_int32(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int32_t, name, short_opt, default_value, help_str, \
                         true)

/**
 * @brief Create a new longname uint32_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_uint32(name, default_value, help_str) \
  DEFINE_flag(uint32_t, name, \0, default_value, help_str, true)

/**
 * @brief Create a new int32_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_uint32(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint32_t, name, short_opt, default_value, help_str, \
                         true)

/**
 * @brief Create a new longname string flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_string(name, default_value, help_str) \
  DEFINE_flag(std::string, name, \0, default_value, help_str, true)

/**
 * @brief Create a new string flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 *     the help screen
 */
#define DEFINE_s_string(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(std::string, name, short_opt, default_value, \
                         help_str, true)

/** @}*/

#endif  // INCLUDE_OLA_BASE_FLAGS_H_
