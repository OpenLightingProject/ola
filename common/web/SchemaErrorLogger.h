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
 * SchemaErrorLogger.h
 * Captures errors while parsing the schema.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef COMMON_WEB_SCHEMAERRORLOGGER_H_
#define COMMON_WEB_SCHEMAERRORLOGGER_H_

#include <ola/base/Macro.h>

#include <ostream>
#include <string>

#include "ola/web/JsonPointer.h"

namespace ola {
namespace web {

/**
 * @brief The SchemaErrorLogger captures errors while parsing the schema.
 *
 * The SchemaErrorLogger allows schema parsing errors to be logged. It prepends
 * the error with the value of the JsonPointer so users have a decent idea of
 * where the error occured in the JSON document.
 */
class SchemaErrorLogger {
 public:
  /**
   * @brief Create a new SchemaErrorLogger.
   * @param pointer the JsonPointer to use when logging error messages
   */
  explicit SchemaErrorLogger(JsonPointer *pointer) : m_pointer(pointer) {}

  /**
   * @brief Check if there was an error logged.
   */
  bool HasError() const;

  /**
   * @brief Return the first error
   * @returns The first error, or the empty string if no error was reported.
   */
  std::string ErrorString() const;

  /**
   * @brief Log an error.
   */
  std::ostream& Error();

  /**
   * @brief Clear the saved errors.
   */
  void Reset();

 private:
  std::ostringstream m_first_error;
  std::ostringstream m_extra_errors;
  JsonPointer *m_pointer;

  DISALLOW_COPY_AND_ASSIGN(SchemaErrorLogger);
};
}  // namespace web
}  // namespace ola
#endif  // COMMON_WEB_SCHEMAERRORLOGGER_H_
