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
 * @brief Convert all seperators in a path to the OS's version.
 * @param path the path to convert
 * @returns the path with all path seperators switched to the OS's version
 */
std::string ConvertPathSeparators(const std::string &path);

/**
 * @brief Join two parts of a path.
 * @param first The first part of the path.
 * @param second The second part of the path.
 * @returns The concatenated path, with the separator if required.
 */
std::string JoinPaths(const std::string &first, const std::string &second);

/**
 * @brief Find all files in a directory that match the given prefix.
 * @param[in] directory the directory to look in
 * @param[in] prefix the prefix to match on
 * @param[out] files a pointer to a vector with the absolute path of the
 * matching files.
 * @returns false if there was an error, true otherwise.
 */
bool FindMatchingFiles(const std::string &directory,
                       const std::string &prefix,
                       std::vector<std::string> *files);

/**
 * @brief Find all files in a directory that match any of the prefixes.
 * @param[in] directory the directory to look in
 * @param[in] prefixes the prefixes to match on
 * @param[out] files a pointer to a vector with the absolute path of the
 * matching files.
 * @returns false if there was an error, true otherwise.
 */
bool FindMatchingFiles(const std::string &directory,
                       const std::vector<std::string> &prefixes,
                       std::vector<std::string> *files);

/**
 * @brief Get a list of all files in a directory.
 *
 * Entries in @param files will contain the full path to the file.
 * @param[in] directory the directory to list
 * @param[out] files a pointer to a string vector that will receive file paths
 * @returns false if there was an error, true otherwise.
 */
bool ListDirectory(const std::string& directory,
                   std::vector<std::string> *files);

/**
 * @brief Convert a path to a filename
 * @param path a full path to a file
 * @param default_value what to return if the path can't be found
 * @return the filename (basename) part of the path or default if it can't be
 *   found
 */
std::string FilenameFromPathOrDefault(const std::string &path,
                                      const std::string &default_value);

/**
 * @brief Convert a path to a filename (this variant is good for switching
 *   based on executable names)
 * @param path a full path to a file
 * @return the filename (basename) part of the path or the whole path if it
 *   can't be found
 */
std::string FilenameFromPathOrPath(const std::string &path);

/**
 * @brief Convert a path to a filename
 * @param path a full path to a file
 * @return the filename (basename) part of the path or "" if it can't be found
 */
std::string FilenameFromPath(const std::string &path);
}  // namespace file
}  // namespace ola
#endif  // INCLUDE_OLA_FILE_UTIL_H_
