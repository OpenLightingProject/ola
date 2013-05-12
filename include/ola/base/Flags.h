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
 *
 * This provides an easy mechanism for handling command line flags. It's
 * based off gflags (https://code.google.com/p/gflags/) but much simpler
 * because we leave out features.
 *
 * Limitations
 *   - Setting flags is not thread safe
 *   - Flags cannot be used at global construction time.
 *   - DEFINE_ and DECLARE_ must be outside of any namespaces.
 *
 * Features:
 *  - bool, (unsigned) int{8,16,32}, string types supported
 *  - short options supported
 *  - inverted bools, e.g. --nofoo
 *
 * Usage:
 *  DEFINE_bool(foo, false, "Enable feature foo");
 *  DEFINE_bool(bar, true, "Disable feature bar");
 *
 *  // Can now use FLAGS_foo and FLAGS_bar, the options are --foo and --nobar.
 *
 *  DEFINE_string(name, "simon", "Specify the name");
 *  // FLAGS_name defaults to "simon" and can be set with --name bob
 *
 *  //Short options can also be specified:
 *  DEFINE_s_int8(baz, b, 0, "Sets the value of baz");
 *  // FLAGS_baz can be set with --baz or -b
 *
 * You can access flags from files other than the one they were DEFINE_*'d in by
 * using DECLARE_*:
 *
 * DECLARE_bool(foo);
 * DECLARE_bool(bar);
 * // Can now use FLAGS_foo and FLAGS_bar
 */
#ifndef INCLUDE_OLA_BASE_FLAGS_H_
#define INCLUDE_OLA_BASE_FLAGS_H_

#include <getopt.h>
#include <ola/base/FlagsPrivate.h>
#include <string>

namespace ola {

using std::string;

/**
 * Set the help string for the program. The first argument is what's displayed
 * after argv[0], the second can be a multi-line description of the program.
 */
void SetHelpString(const string &first_line, const string &description);

/**
 * Print the usage text to stderr.
 */
void DisplayUsage();

/**
 * Parses the command line flags up to the first non-flag value. argv is
 * re-arranged so that it only contains non-flag arguments.
 */
void ParseFlags(int *argc, char **argv);
}  // namespace ola

// DECLARE_*

#define DECLARE_bool(name) \
  DEFINE_flag(bool, name)

#define DECLARE_int8(name) \
  DEFINE_flag(int8_t, name)

#define DECLARE_int16(name) \
  DEFINE_flag(int16_t, name)

#define DECLARE_int32(name) \
  DEFINE_flag(int32_t, name)

#define DECLARE_uint8(name) \
  DEFINE_flag(uint8_t, name)

#define DECLARE_uint16(name) \
  DEFINE_flag(uint16_t, name)

#define DECLARE_uint32(name) \
  DEFINE_flag(uint32_t, name)

// DEFINE_*

#define DEFINE_bool(name, default_value, help_str) \
  DEFINE_flag(bool, name, \0, default_value, help_str)

#define DEFINE_s_bool(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(bool, name, short_opt, default_value, help_str)

#define DEFINE_int8(name, default_value, help_str) \
  DEFINE_flag(int8_t, name, \0, default_value, help_str)

#define DEFINE_s_int8(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int8_t, name, short_opt, default_value, help_str)

#define DEFINE_uint8(name, default_value, help_str) \
  DEFINE_flag(uint8_t, name, \0, default_value, help_str)

#define DEFINE_s_uint8(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint8_t, name, short_opt, default_value, help_str)

#define DEFINE_int16(name, default_value, help_str) \
  DEFINE_flag(int16_t, name, \0, default_value, help_str)

#define DEFINE_s_int16(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int16_t, name, short_opt, default_value, help_str)

#define DEFINE_uint16(name, default_value, help_str) \
  DEFINE_flag(uint16_t, name, \0, default_value, help_str)

#define DEFINE_s_uint16(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint16_t, name, short_opt, default_value, help_str)

#define DEFINE_int32(name, default_value, help_str) \
  DEFINE_flag(int32_t, name, \0, default_value, help_str)

#define DEFINE_s_int32(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(int32_t, name, short_opt, default_value, help_str)

#define DEFINE_uint32(name, default_value, help_str) \
  DEFINE_flag(uint32_t, name, \0, default_value, help_str)

#define DEFINE_s_uint32(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(uint32_t, name, short_opt, default_value, help_str)

#define DEFINE_string(name, default_value, help_str) \
  DEFINE_flag(std::string, name, \0, default_value, help_str)

#define DEFINE_s_string(name, short_opt, default_value, help_str) \
  DEFINE_flag_with_short(std::string, name, short_opt, default_value, help_str)

#endif  // INCLUDE_OLA_BASE_FLAGS_H_
