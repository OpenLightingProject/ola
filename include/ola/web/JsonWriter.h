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
 * JsonWriter.h
 * Serialize JSON data.
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonWriter.h
 * @brief Serialize JSON data.
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONWRITER_H_
#define INCLUDE_OLA_WEB_JSONWRITER_H_

#include <ola/base/Macro.h>
#include <ola/web/Json.h>
#include <stdint.h>
#include <ostream>
#include <string>

namespace ola {
namespace web {

/**
 * @addtogroup json
 * @{
 */

/**
 * @brief A class to serialize a JSONValue to text.
 */
class JsonWriter : public JsonValueVisitorInterface {
 public:
  /**
   * @brief Write the string representation of the JsonValue to a ostream.
   * @param output the ostream to write to
   * @param value the JsonValue to serialize.
   */
  static void Write(std::ostream *output, const JsonValue &value);

  /**
   * @brief Get the string representation of the JsonValue.
   * @param value the JsonValue to serialize.
   */
  static std::string AsString(const JsonValue &value);

  /**
   * @privatesection
   */
  void Visit(const JsonStringValue &value);
  void Visit(const JsonBoolValue &value);
  void Visit(const JsonNullValue &value);
  void Visit(const JsonRawValue &value);
  void Visit(const JsonObject &value);
  void Visit(const JsonArray &value);
  void Visit(const JsonUIntValue &value);
  void Visit(const JsonUInt64Value &value);
  void Visit(const JsonIntValue &value);
  void Visit(const JsonInt64Value &value);
  void Visit(const JsonDoubleValue &value);

  void VisitProperty(const std::string &property, const JsonValue &value);

  /**
   * @endsection
   */

 private:
  std::ostream *m_output;
  unsigned int m_indent;
  std::string m_separator;

  explicit JsonWriter(std::ostream *output)
      : m_output(output),
        m_indent(0),
        m_separator("") {
  }

  /**
   * @brief the default indent level
   */
  static const unsigned int DEFAULT_INDENT = 2;
};
/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONWRITER_H_
