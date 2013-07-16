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
 * Flags.cpp
 * Handle command line flags.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup flags
 * @{
 * @file common/base/Flags.cpp
 * @}
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "ola/base/Flags.h"
#include "ola/base/SysExits.h"
#include "ola/stl/STLUtils.h"

/**
 * @private
 * @brief Define the help flag
 */
DEFINE_s_bool(help, h, false, "Display the help message");

namespace ola {

/**
 * @addtogroup flags
 * @{
 */

using std::cerr;
using std::endl;

/**
 * @brief the prefix used on inverted bool flags
 * @examplepara
 *   @code
 *   DEFINE_s_bool(noMaster, d, false, "Dummy flag to show NO_PREFIX")
 *   @endcode
 *
 *   Then if you called your application with that flag:
 *   @code
 *   bash$myappliation -d
 *   @endcode
 *   Then the noMaster flag would be true.
 */
const char Flag<bool>::NO_PREFIX[] = "no";

void SetHelpString(const string &first_line, const string &description) {
  GetRegistry()->SetFirstLine(first_line);
  GetRegistry()->SetDecription(description);
}


void DisplayUsage() {
  GetRegistry()->DisplayUsage();
}


void ParseFlags(int *argc, char **argv) {
  GetRegistry()->ParseFlags(argc, argv);
}

/**
 * @}
 * @cond HIDDEN_SYMBOLS
 */


/*
 * Change the input to s/_/-/g
 */
void BaseFlag::ReplaceUnderscoreWithHyphen(char *input) {
  for (char *c = input; *c; c++) {
    if (*c == '_')
      *c = '-';
  }
}

/*
 * Convert a flag name to the canonical representation.
 */
const char *BaseFlag::NewCanonicalName(const char *name) {
  unsigned int total_size = strlen(name) + 1;
  char *output = new char[total_size];
  char *o = output;
  for (const char *i = name; *i; i++, o++) {
    if (*i == '_')
      *o = '-';
    else
      *o = *i;
  }
  output[total_size - 1] = 0;
  return output;
}

/**
 * @private
 * Get the global FlagRegistry object.
 */
FlagRegistry *GetRegistry() {
  static FlagRegistry *registry = new FlagRegistry();
  return registry;
}

/**
 * Register a flag.
 */
void FlagRegistry::RegisterFlag(FlagInterface *flag) {
  STLInsertOrDie(&m_long_opts, flag->name(), flag);
  if (flag->short_opt()) {
    STLInsertOrDie(&m_short_opts, flag->short_opt(), flag);
  }
}


/**
 * @brief Parse the command line flags. This re-arranges argv so that only the
 * non-flag options remain.
 */
void FlagRegistry::ParseFlags(int *argc, char **argv) {
  const string long_option_prefix("--");
  const string short_option_prefix("-");


  m_argv0 = argv[0];

  int c;
  int option_index = 0;
  const string short_opts = GetShortOptsString();
  FlagMap flags;
  const struct option *long_options = GetLongOpts(&flags);

  optind = 0;  // reset each time
  while (1) {
    c = getopt_long(*argc, argv, short_opts.c_str(),
                    long_options, &option_index);

    if (c == -1) {
      break;
    } else if (c == '?') {
      exit(EXIT_USAGE);
    }


    FlagInterface *flag = STLFindOrNull(flags, c);
    if (!flag) {
      cerr << "Missing flag " << c << endl;
    } else {
      if (flag->has_arg()) {
        if (!flag->SetValue(optarg)) {
          cerr << "Invalid value " << optarg << endl;
          exit(EXIT_USAGE);
        }
      } else {
        if (!flag->SetValue("1")) {
          cerr << "Invalid value " << optarg << endl;
          exit(EXIT_USAGE);
        }
      }
    }
  }

  if (FLAGS_help) {
    DisplayUsage();
    exit(EXIT_OK);
  }

  delete[] long_options;

  // Remove flags so only non-flag arguments remain.
  for (int i = 0; i < *argc - optind; i++) {
    argv[1 + i] = argv[optind + i];
  }
  *argc = 1 + *argc - optind;
}


void FlagRegistry::SetFirstLine(const string &first_line) {
  m_first_line = first_line;
}


void FlagRegistry::SetDecription(const string &description) {
  m_description = description;
}

/*
 * @brief Print the usage text to stderr
 */
void FlagRegistry::DisplayUsage() {
  cerr << "Usage: " << m_argv0 << " " << m_first_line << endl << endl;
  if (!m_description.empty()) {
    cerr << m_description << endl << endl;
  }

  // - comes before a-z which means flags without long options appear first. To
  // avoid this we keep two lists.
  vector<string> short_flag_lines, long_flag_lines;
  LongOpts::const_iterator iter = m_long_opts.begin();
  for (; iter != m_long_opts.end(); ++iter) {
    std::ostringstream str;
    const FlagInterface *flag = iter->second;
    str << "  ";
    if (flag->short_opt()) {
      str << "-" << flag->short_opt() << ", ";
    }
    str << "--" << flag->name();

    if (flag->has_arg()) {
      str << " <" << flag->arg_type() << ">";
    }
    str << endl << "    " << iter->second->help() << endl;
    if (flag->short_opt())
      short_flag_lines.push_back(str.str());
    else
      long_flag_lines.push_back(str.str());
  }

  PrintFlags(&short_flag_lines);
  PrintFlags(&long_flag_lines);
}

/**
 * @brief Generate the short opts string for getopt. See `man 3 getopt` for the
 * format.
 */
string FlagRegistry::GetShortOptsString() const {
  string short_opts;
  ShortOpts::const_iterator iter = m_short_opts.begin();
  for (; iter != m_short_opts.end(); ++iter) {
    char short_opt = iter->second->short_opt();
    if (!short_opt)
      continue;

    short_opts.push_back(iter->second->short_opt());
    if (iter->second->has_arg()) {
      short_opts.push_back(':');
    }
  }
  return short_opts;
}

/**
 * @brief Allocate & populate the array of option structs for the call to
 * getopt_long. The caller is responsible for deleting the array.o
 *
 * The flag_map is populated with the option identifier (int) to FlagInterface*
 * mappings. The ownership of the pointers to FlagInterfaces is not transferred
 * to the caller.
 */
struct option *FlagRegistry::GetLongOpts(FlagMap *flag_map) {
  unsigned int flag_count = m_long_opts.size() + 1;
  struct option *long_options = new struct option[flag_count];
  memset(long_options, 0, sizeof(struct option) * flag_count);

  LongOpts::iterator iter = m_long_opts.begin();
  struct option *opt = long_options;
  int flag_counter = 256;
  for (; iter != m_long_opts.end(); ++iter) {
    FlagInterface *flag = iter->second;
    opt->name = flag->name();
    opt->has_arg = flag->has_arg() ? required_argument : no_argument;
    opt->flag = 0;
    int flag_option = flag->short_opt() ? flag->short_opt() : flag_counter++;
    opt->val = flag_option;
    flag_map->insert(FlagMap::value_type(flag_option, flag));
    opt++;
  }
  return long_options;
}


/**
 * @brief Given a vector of flags lines, sort them and print to stderr.
 */
void FlagRegistry::PrintFlags(std::vector<string> *lines) {
  std::sort(lines->begin(), lines->end());
  vector<string>::const_iterator iter = lines->begin();
  for (; iter != lines->end(); ++iter)
    cerr << *iter;
}
/**
 * @endcond
 * End Hidden Symbols
 */
}  // namespace ola
