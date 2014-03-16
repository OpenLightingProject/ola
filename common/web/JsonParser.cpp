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
 * Copyright (C) 2013 Simon Newton
 */
#include "ola/web/JsonParser.h"

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include <stdint.h>
#include <math.h>

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

static JsonValue *ParseTrimedInput(const char **input, string *error);

/**
 * @brief Trim leading whitespace from a string.
 * @param input A pointer to a pointer with the data. This is updated to point
 * to the first non-whitespace character.
 * @returns true if more data remains, false if the string is now empty.
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
static bool ParseString(const char **input, string* str) {
  while (true) {
    size_t size = strcspn(*input, "\"\\");
    char c = (*input)[size];
    if (c == 0) {
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
          // *str = "Invalid escape character: \\" << **input;
          return false;
      }
      str->push_back(append_char);
      *input += 1;
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
static JsonValue* ParseNumber(const char **input) {
  bool is_decimal = false;
  bool is_negative = (**input == '-');
  if (is_negative) {
    (*input)++;
    if (**input == 0) {
      return NULL;
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
    return NULL;
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
      return NULL;
    }

    uint64_t unsignend_exponent;
    if (isdigit(**input)) {
      ExtractDigits(input, &unsignend_exponent);
    } else {
      return NULL;
    }
    int64_t exponent = (negative_exponent ? -1 : 1) * unsignend_exponent;

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
    return new JsonDoubleValue(d);
  } else if (is_negative) {
    int64_t value = -1 * i;
    if (value < INT32_MIN || value > INT32_MAX) {
      return new JsonInt64Value(value);
    } else {
      return new JsonIntValue(value);
    }
  } else {
    if (i > UINT32_MAX) {
      return new JsonUInt64Value(i);
    } else {
      return new JsonUIntValue(i);
    }
  }
}

/**
 * Starts from the first character after the  '['.
 */
static JsonValue* ParseArray(const char **input, string *error) {
  if (!TrimWhitespace(input)) {
    return NULL;
  }

  if (**input == ']') {
    (*input)++;
    return new JsonArray();
  }

  auto_ptr<JsonArray> array(new JsonArray());

  while (true) {
    if (!TrimWhitespace(input)) {
      return NULL;
    }

    auto_ptr<JsonValue> value(ParseTrimedInput(input, error));
    if (value.get()) {
      array->Append(value.release());
    } else {
      return NULL;
    }

    if (!TrimWhitespace(input)) {
      return NULL;
    }

    switch (**input) {
      case ']':
        (*input)++;  // move past the ]
        return array.release();
      case ',':
        break;
      default:
        return NULL;
    }
    (*input)++;  // move past the ,
  }
}

/**
 * Starts from the first character after the  '{'.
 */
static JsonValue* ParseObject(const char **input, string *error) {
  if (!TrimWhitespace(input)) {
    return NULL;
  }

  if (**input == '}') {
    (*input)++;
    return new JsonObject();
  }

  auto_ptr<JsonObject> object(new JsonObject());

  while (true) {
    if (!TrimWhitespace(input)) {
      return NULL;
    }

    if (**input != '"') {
      return NULL;
    }
    (*input)++;

    string key;
    if (!ParseString(input, &key)) {
      return NULL;
    }
    OLA_INFO << "input is now . " << *input << ".";

    if (!TrimWhitespace(input)) {
      return NULL;
    }

    if (**input != ':') {
      return NULL;
    }
    (*input)++;

    if (!TrimWhitespace(input)) {
      return NULL;
    }

    auto_ptr<JsonValue> value(ParseTrimedInput(input, error));
    if (value.get()) {
      object->AddValue(key, value.release());
    } else {
      return NULL;
    }

    if (!TrimWhitespace(input)) {
      return NULL;
    }

    switch (**input) {
      case '}':
        (*input)++;  // move past the }
        return object.release();
      case ',':
        break;
      default:
        return NULL;
    }
    (*input)++;  // move past the ,
  }
}

/**
 *
 */
static JsonValue *ParseTrimedInput(const char **input, string *error) {
  static const char TRUE_STR[] = "true";
  static const char FALSE_STR[] = "false";
  static const char NULL_STR[] = "null";

  if (**input == '"') {
    (*input)++;
    string str;
    if (ParseString(input, &str)) {
      return new JsonStringValue(str);
    }
    return NULL;
  } else if (strncmp(*input, TRUE_STR, sizeof(TRUE_STR) - 1) == 0) {
    *input += sizeof(TRUE_STR) - 1;
    return new JsonBoolValue(true);
  } else if (strncmp(*input, FALSE_STR, sizeof(FALSE_STR) - 1) == 0) {
    *input += sizeof(FALSE_STR) - 1;
    return new JsonBoolValue(false);
  } else if (strncmp(*input, NULL_STR, sizeof(NULL_STR) - 1) == 0) {
    *input += sizeof(NULL_STR) - 1;
    return new JsonNullValue();
  } else if (**input == '-' || isdigit(**input)) {
    return ParseNumber(input);
  } else if (**input == '[') {
    (*input)++;
    return ParseArray(input, error);
  } else if (**input == '{') {
    (*input)++;
    return ParseObject(input, error);
  }
  return NULL;
}

JsonValue* JsonParser::Parse(const std::string &input, string *error) {
  // TODO(simon): Do we need to convert to unicode here? I think this may be
  // an issue on Windows.
  OLA_INFO << "-------------------";
  char* input_data = new char[input.size() + 1];
  OLA_INFO << "input size is " << input.size() << ", " << input;
  strncpy(input_data, input.c_str(), input.size() + 1);
  OLA_INFO << "copied size is " << strlen(input_data);

  JsonValue *value = ParseRaw(input_data, error);
  delete[] input_data;
  return value;
  (void) error;
}


JsonValue* JsonParser::ParseRaw(const char *input, string *error) {
  if (!TrimWhitespace(&input)) {
    return NULL;
  }

  auto_ptr<JsonValue> value(ParseTrimedInput(&input, error));
  if (TrimWhitespace(&input)) {
    return NULL;
  }
  return value.release();
}
}  // namespace web
}  // namespace ola
