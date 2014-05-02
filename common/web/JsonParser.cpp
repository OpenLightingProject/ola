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
 * JsonParser.cpp
 * A Json Parser.
 * See http://www.json.org/
 * Copyright (C) 2014 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include "ola/web/JsonParser.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

using std::auto_ptr;
using std::string;
using std::stringstream;

static bool ParseTrimmedInput(const char **input,
                              JsonHandlerInterface *handler);

/**
 * @brief Trim leading whitespace from a string.
 * @param input A pointer to a pointer with the data. This is updated to point
 * to the first non-whitespace character.
 * @returns true if more data remains, false if the string is now empty.
 * @note This is static within JsonParser.cpp because we should be using
 * strings rather than char[]s. The only reason this doesn't use a string is
 * because I think we'll need a wchar on Windows.
 */
static bool TrimWhitespace(const char **input) {
  while (**input != 0 &&
         (**input == ' ' || **input == '\t' || **input == '\r' ||
          **input == '\n')) {
    (*input)++;
  }
  return **input != 0;
}

/**
 * @brief Extract a string token from the input.
 * @param input A pointer to a pointer with the data. This should point to the
 * first character after the quote (") character.
 * @param str A string object to store the extracted string.
 * @returns true if the string was extracted correctly, false otherwise.
 */
static bool ParseString(const char **input, string* str,
                        JsonHandlerInterface *handler) {
  while (true) {
    size_t size = strcspn(*input, "\"\\");
    char c = (*input)[size];
    if (c == 0) {
      handler->SetError("Unterminated string");
      str->clear();
      return false;
    }

    str->append(*input, size);
    *input += size + 1;

    if (c == '"') {
      return true;
    }

    if (c == '\\') {
      // escape character
      char append_char = 0;

      switch (**input) {
        case '"':
        case '\\':
        case '/':
          append_char = **input;
          break;
        case 'b':
          append_char = '\b';
          break;
        case 'f':
          append_char = '\f';
          break;
        case 'n':
          append_char = '\n';
          break;
        case 'r':
          append_char = '\r';
          break;
        case 't':
          append_char = '\t';
          break;
        case 'u':
          // TODO(simonn): handle unicode
          OLA_INFO << "unicode character found";
          break;
        default:
          OLA_WARN << "Invalid escape character: \\" << **input;
          handler->SetError("Invalid string escape sequence");
          return false;
      }
      str->push_back(append_char);
      (*input)++;
    }
  }
  return true;
}

bool ExtractDigits(const char **input, uint64_t *i,
                   unsigned int *places = NULL) {
  *i = 0;
  unsigned int decimal_places = 0;
  while (isdigit(**input)) {
    *i *= 10;
    *i += **input - '0';
    decimal_places++;
    (*input)++;
  }
  if (places) {
    *places = decimal_places;
  }
  return true;
}

/**
 * Parse a JsonNumber
 */
static bool ParseNumber(const char **input, JsonHandlerInterface *handler) {
  bool is_decimal = false;
  bool is_negative = (**input == '-');
  if (is_negative) {
    (*input)++;
    if (**input == 0) {
      return false;
    }
  }

  long double d = 0.0;

  // Extract the int component
  uint64_t i = 0;

  if (**input == '0') {
    (*input)++;
  } else if (isdigit(**input)) {
    ExtractDigits(input, &i);
  } else {
    return false;
  }

  if (**input == '.') {
    // Handle decimal places
    (*input)++;
    uint64_t decimal;
    unsigned int decimal_places;
    ExtractDigits(input, &decimal, &decimal_places);
    d = i + static_cast<double>(decimal) / pow(10, decimal_places);
    if (is_negative) {
      d *= -1;
    }
    is_decimal = true;
  }

  if (**input == 'e' || **input == 'E') {
    // Handle the exponent
    (*input)++;

    bool negative_exponent = false;
    switch (**input) {
      case '-':
        negative_exponent = true;
      case '+':
        (*input)++;
        break;
      default:
        {};
    }

    if (**input == 0) {
      return false;
    }

    uint64_t unsigned_exponent;
    if (isdigit(**input)) {
      ExtractDigits(input, &unsigned_exponent);
    } else {
      return false;
    }
    int64_t exponent = (negative_exponent ? -1 : 1) * unsigned_exponent;

    if (is_decimal) {
      d *= pow(10, exponent);
    } else if (exponent >= 0) {
      i *= pow(10, exponent);
    } else {
      // If the exponent is negative, we need to switch to a decimal
      d = i * pow(10, exponent);
      if (is_negative) {
        d *= -1;
      }
      is_decimal = true;
    }
  }

  if (is_decimal) {
    if (fabs(d) == 0.0) {
      d = 0.0;
    }
    handler->Number(d);
  } else if (is_negative) {
    int64_t value = -1 * i;
    if (value < INT32_MIN || value > INT32_MAX) {
      handler->Number(value);
    } else {
      handler->Number(static_cast<int32_t>(value));
    }
  } else {
    if (i > UINT32_MAX) {
      handler->Number(i);
    } else {
      handler->Number(static_cast<uint32_t>(i));
    }
  }
  return true;
}

/**
 * Starts from the first character after the  '['.
 */
static bool ParseArray(const char **input, JsonHandlerInterface *handler) {
  if (!TrimWhitespace(input)) {
    handler->SetError("Unterminated array");
    return false;
  }

  handler->OpenArray();

  if (**input == ']') {
    (*input)++;
    handler->CloseArray();
    return true;
  }

  while (true) {
    if (!TrimWhitespace(input)) {
      handler->SetError("Unterminated array");
      return false;
    }

    bool result = ParseTrimmedInput(input, handler);
    if (!result) {
      OLA_INFO << "input failed";
      return false;
    }

    if (!TrimWhitespace(input)) {
      handler->SetError("Unterminated array");
      return false;
    }

    switch (**input) {
      case ']':
        (*input)++;  // move past the ]
        handler->CloseArray();
        return true;
      case ',':
        break;
      default:
        handler->SetError("Expected either , or ] after an array element");
        return false;
    }
    (*input)++;  // move past the ,
  }
}

/**
 * Starts from the first character after the  '{'.
 */
static bool ParseObject(const char **input, JsonHandlerInterface *handler) {
  if (!TrimWhitespace(input)) {
    handler->SetError("Unterminated object");
    return false;
  }

  handler->OpenObject();

  if (**input == '}') {
    (*input)++;
    handler->CloseObject();
    return true;
  }

  while (true) {
    if (!TrimWhitespace(input)) {
      handler->SetError("Unterminated object");
      return false;
    }

    if (**input != '"') {
      handler->SetError("Expected key for object");
      OLA_INFO << "Expected string";
      return false;
    }
    (*input)++;

    string key;
    if (!ParseString(input, &key, handler)) {
      return false;
    }
    handler->ObjectKey(key);

    if (!TrimWhitespace(input)) {
      handler->SetError("Missing : after key");
      return false;
    }

    if (**input != ':') {
      handler->SetError("Incorrect character after key, should be :");
      return false;
    }
    (*input)++;

    if (!TrimWhitespace(input)) {
      handler->SetError("Unterminated object");
      return false;
    }

    bool result = ParseTrimmedInput(input, handler);
    if (!result) {
      return false;
    }

    if (!TrimWhitespace(input)) {
      handler->SetError("Unterminated object");
      return false;
    }

    switch (**input) {
      case '}':
        (*input)++;  // move past the }
        handler->CloseObject();
        return true;
      case ',':
        break;
      default:
        handler->SetError("Expected either , or } after an object value");
        return false;
    }
    (*input)++;  // move past the ,
  }
}

static bool ParseTrimmedInput(const char **input,
                             JsonHandlerInterface *handler) {
  static const char TRUE_STR[] = "true";
  static const char FALSE_STR[] = "false";
  static const char NULL_STR[] = "null";

  if (**input == '"') {
    (*input)++;
    string str;
    if (ParseString(input, &str, handler)) {
      handler->String(str);
      return true;
    }
    return false;
  } else if (strncmp(*input, TRUE_STR, sizeof(TRUE_STR) - 1) == 0) {
    *input += sizeof(TRUE_STR) - 1;
    handler->Bool(true);
    return true;
  } else if (strncmp(*input, FALSE_STR, sizeof(FALSE_STR) - 1) == 0) {
    *input += sizeof(FALSE_STR) - 1;
    handler->Bool(false);
    return true;
  } else if (strncmp(*input, NULL_STR, sizeof(NULL_STR) - 1) == 0) {
    *input += sizeof(NULL_STR) - 1;
    handler->Null();
    return true;
  } else if (**input == '-' || isdigit(**input)) {
    return ParseNumber(input, handler);
  } else if (**input == '[') {
    (*input)++;
    return ParseArray(input, handler);
  } else if (**input == '{') {
    (*input)++;
    return ParseObject(input, handler);
  }
  handler->SetError("Invalid JSON value");
  return false;
}


bool ParseRaw(const char *input, JsonHandlerInterface *handler) {
  if (!TrimWhitespace(&input)) {
    handler->SetError("No JSON data found");
    return false;
  }

  handler->Begin();
  bool result = ParseTrimmedInput(&input, handler);
  if (!result) {
    return false;
  }
  handler->End();
  return !TrimWhitespace(&input);
}

bool JsonParser::Parse(const std::string &input,
                       JsonHandlerInterface *handler) {
  // TODO(simon): Do we need to convert to unicode here? I think this may be
  // an issue on Windows. Consider mbstowcs.
  // Copying the input sucks though, so we should use input.c_str() if we can.
  char* input_data = new char[input.size() + 1];
  strncpy(input_data, input.c_str(), input.size() + 1);

  bool result = ParseRaw(input_data, handler);
  delete[] input_data;
  return result;
}
}  // namespace web
}  // namespace ola
