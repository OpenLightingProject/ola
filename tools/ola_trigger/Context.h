/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Context.h
 * Copyright (C) 2011 Simon Newton
 */


#ifndef TOOLS_OLA_TRIGGER_CONTEXT_H_
#define TOOLS_OLA_TRIGGER_CONTEXT_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <stdint.h>
#include <sstream>
#include <string>
#include HASH_MAP_H

#ifndef HAVE_UNORDERED_MAP
// This adds support for hashing strings if it's not present
namespace HASH_NAMESPACE {

template<> struct hash<std::string> {
  size_t operator()(const std::string& x) const {
    return hash<const char*>()(x.c_str());
  }
};
}  // namespace HASH_NAMESPACE
#endif  // HAVE_UNORDERED_MAP

/**
 * @brief A context is a collection of variables and their values.
 */
class Context {
 public:
  Context() {}
  ~Context();

  bool Lookup(const std::string &name, std::string *value) const;
  void Update(const std::string &name, const std::string &value);

  void SetConfigFile(const std::string &config_file);
  void SetOverallOffset(uint16_t overall_offset);
  void SetSlotValue(uint8_t value);
  void SetSlotOffset(uint16_t offset);
  void SetUniverse(uint32_t universe);

  std::string AsString() const;
  friend std::ostream& operator<<(std::ostream &out, const Context&);

  static const char CONFIG_FILE_VARIABLE[];
  static const char OVERALL_OFFSET_VARIABLE[];
  static const char SLOT_VALUE_VARIABLE[];
  static const char SLOT_OFFSET_VARIABLE[];
  static const char UNIVERSE_VARIABLE[];

 private:
  typedef HASH_NAMESPACE::HASH_MAP_CLASS<std::string,
                                         std::string> VariableMap;
  VariableMap m_variables;
};
#endif  // TOOLS_OLA_TRIGGER_CONTEXT_H_
