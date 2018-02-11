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
 * Util.cpp
 * File related helper functions.
 * Copyright (C) 2013 Simon Newton
 */

#include <ola/Logging.h>
#include <ola/file/Util.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <ola/win/CleanWindows.h>
#endif  // _WIN32

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace ola {
namespace file {

using std::string;
using std::vector;

#ifdef _WIN32
const char PATH_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = '/';
#endif  // _WIN32

string ConvertPathSeparators(const string &path) {
  string result = path;
#ifdef _WIN32
  std::replace(result.begin(), result.end(), '/', PATH_SEPARATOR);
#else
  std::replace(result.begin(), result.end(), '\\', PATH_SEPARATOR);
#endif  // _WIN32
  return result;
}

string JoinPaths(const string &first, const string &second) {
  if (second.empty()) {
    return first;
  }

  if (first.empty()) {
    return second;
  }

  if (second[0] == PATH_SEPARATOR) {
    return second;
  }

  string output(first);
  if (output[output.size() - 1] != PATH_SEPARATOR) {
    output.push_back(PATH_SEPARATOR);
  }
  output.append(second);
  return output;
}

bool FindMatchingFiles(const string &directory,
                       const string &prefix,
                       vector<string> *files) {
  vector<string> prefixes;
  prefixes.push_back(prefix);
  return FindMatchingFiles(directory, prefixes, files);
}

bool FindMatchingFiles(const string &directory,
                       const vector<string> &prefixes,
                       vector<string> *files) {
  if (directory.empty() || prefixes.empty()) {
    return true;
  }

#ifdef _WIN32
  WIN32_FIND_DATA find_file_data;
  HANDLE h_find;
  string mutable_directory = ConvertPathSeparators(directory);

  // Strip trailing path separators, otherwise FindFirstFile fails
  while (*mutable_directory.rbegin() == PATH_SEPARATOR) {
    mutable_directory.erase(mutable_directory.size() - 1);
  }

  string search_pattern = mutable_directory + PATH_SEPARATOR + "*";

  h_find = FindFirstFileA(search_pattern.data(), &find_file_data);
  if (h_find == INVALID_HANDLE_VALUE) {
    OLA_WARN << "Find first file failed: " << GetLastError() << " for "
             << search_pattern;
    return false;
  }

  do {
    vector<string>::const_iterator iter;
    for (iter = prefixes.begin(); iter != prefixes.end(); ++iter) {
      if (!strncmp(find_file_data.cFileName, iter->data(), iter->size())) {
        std::ostringstream str;
        str << mutable_directory << PATH_SEPARATOR << find_file_data.cFileName;
        files->push_back(str.str());
      }
    }
  } while (FindNextFile(h_find, &find_file_data));

  FindClose(h_find);
#else
  DIR *dp;
  struct dirent dir_ent;
  struct dirent *dir_ent_p;
  if ((dp = opendir(directory.data())) == NULL) {
    OLA_WARN << "Could not open " << directory << ":" << strerror(errno);
    return false;
  }

  if (readdir_r(dp, &dir_ent, &dir_ent_p)) {
    OLA_WARN << "readdir_r(" << directory << "): " << strerror(errno);
    closedir(dp);
    return false;
  }

  while (dir_ent_p != NULL) {
    vector<string>::const_iterator iter;
    for (iter = prefixes.begin(); iter != prefixes.end(); ++iter) {
      if (!strncmp(dir_ent_p->d_name, iter->data(), iter->size())) {
        std::ostringstream str;
        str << directory << PATH_SEPARATOR << dir_ent_p->d_name;
        files->push_back(str.str());
      }
    }
    if (readdir_r(dp, &dir_ent, &dir_ent_p)) {
      OLA_WARN << "readdir_r(" << directory << "): " << strerror(errno);
      closedir(dp);
      return false;
    }
  }
  if (closedir(dp)) {
    OLA_WARN << "closedir(" << directory << "): " << strerror(errno);
    return false;
  }
#endif  // _WIN32
  return true;
}

bool ListDirectory(const string& directory,
                   vector<string> *files) {
  return FindMatchingFiles(directory, "", files);
}

string FilenameFromPathOrDefault(const string &path,
                                 const string &default_value) {
  string mutable_path = ConvertPathSeparators(path);
  string::size_type last_path_sep = string::npos;
  last_path_sep = mutable_path.find_last_of(PATH_SEPARATOR);
  if (last_path_sep == string::npos)
    return default_value;
  // Don't return the path sep itself
  return mutable_path.substr(last_path_sep + 1);
}

string FilenameFromPathOrPath(const string &path) {
  return FilenameFromPathOrDefault(path, path);
}

string FilenameFromPath(const string &path) {
  return FilenameFromPathOrDefault(path, "");
}
}  // namespace file
}  // namespace ola
