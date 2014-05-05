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
#include <Windows.h>
#endif

#if HAVE_CONFIG_H
#include <config.h>
#endif

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
#endif

void FindMatchingFiles(const string &directory,
                       const string &prefix,
                       vector<string> *files) {
  vector<string> prefixes;
  prefixes.push_back(prefix);
  FindMatchingFiles(directory, prefixes, files);
}

void FindMatchingFiles(const string &directory,
                       const vector<string> &prefixes,
                       vector<string> *files) {
  if (directory.empty() || prefixes.empty())
    return;

#ifdef _WIN32
  WIN32_FIND_DATA find_file_data;
  HANDLE h_find;
  string mutable_directory = directory;

  // Strip trailing path separators, otherwise FindFirstFile fails
  while ((*mutable_directory.rbegin() == '\\') ||
      (*mutable_directory.rbegin() == '/')) {
    mutable_directory.erase(mutable_directory.size() - 1);
  }

  string search_pattern = mutable_directory + PATH_SEPARATOR + "*";

  h_find = FindFirstFileA(search_pattern.data(), &find_file_data);
  if (h_find == INVALID_HANDLE_VALUE) {
    OLA_WARN << "Find first file failed: " << GetLastError();
    return;
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
  if ((dp  = opendir(directory.data())) == NULL) {
    OLA_WARN << "Could not open " << directory << ":" << strerror(errno);
    return;
  }

  readdir_r(dp, &dir_ent, &dir_ent_p);
  while (dir_ent_p != NULL) {
    vector<string>::const_iterator iter;
    for (iter = prefixes.begin(); iter != prefixes.end(); ++iter) {
      if (!strncmp(dir_ent_p->d_name, iter->data(), iter->size())) {
        std::ostringstream str;
        str << directory << PATH_SEPARATOR << dir_ent_p->d_name;
        files->push_back(str.str());
      }
    }
    readdir_r(dp, &dir_ent, &dir_ent_p);
  }
  closedir(dp);
#endif
}

void ListDirectory(const std::string& directory,
                   std::vector<std::string> *files) {
  FindMatchingFiles(directory, "", files);
}

string FilenameFromPathOrDefault(const string &path,
                                 const string &default_value) {
  string::size_type last_path_sep = string::npos;
  last_path_sep = path.find_last_of(PATH_SEPARATOR);
  if (last_path_sep == string::npos)
    return default_value;
  return path.substr(last_path_sep + 1);  // Don't return the path sep itself
}

string FilenameFromPathOrPath(const string &path) {
  return FilenameFromPathOrDefault(path, path);
}

string FilenameFromPath(const string &path) {
  return FilenameFromPathOrDefault(path, "");
}
}  // namespace file
}  // namespace ola
