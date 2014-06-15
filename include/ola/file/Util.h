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
 * Util.h
 * File related helper functions.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_FILE_UTIL_H_
#define INCLUDE_OLA_FILE_UTIL_H_

#include <string>
#include <vector>

namespace ola {
namespace file {

extern const char PATH_SEPARATOR;

/**
 * Find all files in a directory that match the given prefix.
 * @param[in] directory the directory to look in
 * @param[in] prefix the prefix to match on
 * @param[out] files a pointer to a vector with the absolute path of the
 * matching files.
 */
void FindMatchingFiles(const std::string &directory,
                       const std::string &prefix,
                       std::vector<std::string> *files);

/**
 * Find all files in a directory that match any of the prefixes.
 * @param[in] directory the directory to look in
 * @param[in] prefixes the prefixes to match on
 * @param[out] files a pointer to a vector with the absolute path of the
 * matching files.
 */
void FindMatchingFiles(const std::string &directory,
                       const std::vector<std::string> &prefixes,
                       std::vector<std::string> *files);

/**
 * Get a list of all files in a directory. Entries in \p files will contain
 * the full path to the file.
 * @param[in] directory the directory to list
 * @param[out] files a pointer to a string vector that will receive file paths
 */
void ListDirectory(const std::string& directory,
                   std::vector<std::string> *files);

/**
 * Convert a path to a filename
 * @param path a full path to a file
 * @param default_value what to return if the path can't be found
 * @return the filename (basename) part of the path or default if it can't be
 *   found
 */
std::string FilenameFromPathOrDefault(const std::string &path,
                                      const std::string &default_value);

/**
 * Convert a path to a filename (this variant is good for switching based on
 *   executable names)
 * @param path a full path to a file
 * @return the filename (basename) part of the path or the whole path if it
 *   can't be found
 */
std::string FilenameFromPathOrPath(const std::string &path);

/**
 * Convert a path to a filename
 * @param path a full path to a file
 * @return the filename (basename) part of the path or "" if it can't be found
 */
std::string FilenameFromPath(const std::string &path);
}  // namespace file
}  // namespace ola
#endif  // INCLUDE_OLA_FILE_UTIL_H_
