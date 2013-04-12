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

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "common/base/SystemExits.h"
#include "ola/base/Flags.h"
#include "ola/stl/STLUtils.h"

DEFINE_s_bool(help, h, false, "Display the help message");

namespace ola {

using std::cerr;
using std::endl;

const char Flag<bool>::NO_PREFIX[] = "no";

/**
 * Set the help string for the program. The first argument is what's displayed
 * after argv[0], the second can be a multi-line description of the program.
 */
void SetHelpString(const string &first_line, const string &description) {
  GetRegistry()->SetFirstLine(first_line);
  GetRegistry()->SetDecription(description);
}

/**
 * Print the usage text to stderr.
 */
void DisplayUsage() {
  GetRegistry()->DisplayUsage();
}

/**
 * Parses the command line flags up to the first non-flag value. argv is
 * re-arranged so that it only contains non-flag arguments.
 */
void ParseFlags(int *argc, char **argv) {
  GetRegistry()->ParseFlags(argc, argv);
}

/**
 * Change the input to s/_/-/g
 */
void BaseFlag::ReplaceUnderscoreWithHyphen(char *input) {
  for (char *c = input; *c; c++) {
    if (*c == '_')
      *c = '-';
  }
}

/**
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


/*
 * Parse the command line flags. This re-arranges argv so that only the
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
      exit(EX_USAGE);
    }


    FlagInterface *flag = STLFindOrNull(flags, c);
    if (!flag) {
      cerr << "Missing flag " << c << endl;
    } else {
      if (flag->has_arg()) {
        if (!flag->SetValue(optarg)) {
          cerr << "Invalid value " << optarg << endl;
          exit(EX_USAGE);
        }
      } else {
        if (!flag->SetValue("1")) {
          cerr << "Invalid value " << optarg << endl;
          exit(EX_USAGE);
        }
      }
    }
  }

  if (FLAGS_help) {
    DisplayUsage();
    exit(EX_OK);
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

/**
 * Print the usage text to stderr
 */
void FlagRegistry::DisplayUsage() {
  cerr << "Usage: " << m_argv0 << " " << m_first_line << endl << endl;
  if (!m_description.empty()) {
    cerr << m_description << endl << endl;
  }

  vector<string> lines;
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
    lines.push_back(str.str());
  }
  std::sort(lines.begin(), lines.end());
  vector<string>::const_iterator line_iter = lines.begin();
  for (; line_iter != lines.end(); ++line_iter) {
    cerr << *line_iter;
  }
}

/**
 * Generate the short opts string for getopt. See `man 3 getopt` for the format.
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
 * Allocate & populate the array of option structs for the call to getopt_long.
 * The caller is responsible for deleting the array.o
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
}  // ola
