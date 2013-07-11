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
 *  - inverted bools, e.g. --nofoo
 *
 * @note
 *   - Setting flags is not thread safe
 *   - Flags cannot be used at global construction time.
 *   - DEFINE_ and DECLARE_ must be outside of any namespaces.
 *
 * @snippet
 *  @code
 *  // These options are --foo and --nobar.
 *  DEFINE_bool(foo, false, "Enable feature foo");
 *  DEFINE_bool(bar, true, "Disable feature bar");
 *
 *  // FLAGS_name defaults to "simon" and can be changed with --name bob
 *  DEFINE_string(name, "simon", "Specify the name");
 *
 *  // Short options can also be specified:
 *  // FLAGS_baz can be set with --baz or -b
 *  DEFINE_s_int8(baz, b, 0, "Sets the value of baz");
 *
 *  int main(int argc, char* argv[]) {
 *    ola::SetHelpString("<options>", "Description of binary");
 *    ola::ParseFlags(argc, argv);
 *
 *    cout << "--foo is " << FLAGS_foo << endl;
 *    cout << "--bar is " << FLAGS_bar << endl;
 *    cout << "--name is " << FLAGS_name.str() << endl;
 *    cout << "--baz (-b) is " << FLAGS_baz << endl;
 *  }
 *  @endcode
 *
 * @snippet - Use flags from other files
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

using std::string;

/**
 * @brief Set the help string for the program.
 * @param first_line the inital line that is displayed in the help section.
 * This is displayed after argv[0].
 * @param description a multiline description of the program
 *
 */
void SetHelpString(const string &first_line, const string &description);

/**
 * @brief Print the usage text to stderr.
 */
void DisplayUsage();

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
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * Either true, or false.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_bool(name, default_value, help_str) \
  DEFINE_flag(bool, name, \0, default_value, help_str)

/**
 * @brief Create a new flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * Either true, or false
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_bool(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(bool, name, short_opt, default_value, help_str)

/**
 * @brief Create a new longname int8_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_int8(name, default_value, help_str) \
  DEFINE_flag(int8_t, name, \0, default_value, help_str)

/**
 * @brief Create a new int8_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_int8(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int8_t, name, short_opt, default_value, help_str)

/**
 * @brief Create a new longname uint8_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_uint8(name, default_value, help_str) \
  DEFINE_flag(uint8_t, name, \0, default_value, help_str)

/**
 * @brief Create a new uint8_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_uint8(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint8_t, name, short_opt, default_value, help_str)

/**
 * @brief Create a new longname int16_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_int16(name, default_value, help_str) \
  DEFINE_flag(int16_t, name, \0, default_value, help_str)

/**
 * @brief Create a new int16_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_int16(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int16_t, name, short_opt, default_value, help_str)


/**
 * @brief Create a new longname uint16_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_uint16(name, default_value, help_str) \
  DEFINE_flag(uint16_t, name, \0, default_value, help_str)

/**
 * @brief Create a new uint16_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_uint16(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint16_t, name, short_opt, default_value, help_str)

/**
 * @brief Create a new longname int32_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_int32(name, default_value, help_str) \
  DEFINE_flag(int32_t, name, \0, default_value, help_str)

/**
 * @brief Create a new int32_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_int32(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int32_t, name, short_opt, default_value, help_str)

/**
 * @brief Create a new longname uint32_t flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_uint32(name, default_value, help_str) \
  DEFINE_flag(uint32_t, name, \0, default_value, help_str)

/**
 * @brief Create a new int32_t flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_uint32(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint32_t, name, short_opt, default_value, help_str)

/**
 * @brief Create a new longname string flag
 * @param name the name of the flag to create
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_string(name, default_value, help_str) \
  DEFINE_flag(std::string, name, \0, default_value, help_str)

/**
 * @brief Create a new string flag with a long and short name
 * @param name the full name of the flag to create
 * @param short_opt the short name of the flag. For example "-h", or "-d".
 * @param default_value the default value for the flag.
 * @param help_str the string displayed when the program is asked to display
 * the help screen
 */
#define DEFINE_s_string(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(std::string, name, short_opt, default_value, help_str)

/** @}*/

#endif  // INCLUDE_OLA_BASE_FLAGS_H_
