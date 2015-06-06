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
 * JsonLexer.cpp
 * A Json Lexer.
 * See http://www.json.org/
 * Copyright (C) 2014 Simon Newton
 */

#define __STDC_LIMIT_MACROS  // for UINT8_MAX & friends

#include "ola/web/JsonLexer.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <memory>
#include <string>
#include "ola/Logging.h"
#include "ola/web/Json.h"

namespace ola {
namespace web {

using std::auto_ptr;
using std::string;

static bool ParseTrimmedInput(const char **input,
                              JsonParserInterface *parser);

/**
 * @brief Trim leading whitespace from a string.
 * @param input A pointer to a pointer with the data. This is updated to point
 * to the first non-whitespace character.
 * @returns true if more data remains, false if the string is now empty.
 * @note This is static within JsonLexer.cpp because we should be using
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
 * @param parser the JsonParserInterface to pass tokens to.
 * @returns true if the string was extracted correctly, false otherwise.
 */
static bool ParseString(const char **input, string* str,
                        JsonParserInterface *parser) {
  while (true) {
    size_t size = strcspn(*input, "\"\\");
    char c = (*input)[size];
    if (c == 0) {
      parser->SetError("Unterminated string");
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
          parser->SetError("Invalid string escape sequence");
          return false;
      }
      str->push_back(append_char);
      (*input)++;
    }
  }
  return true;
}

bool ExtractDigits(const char **input, uint64_t *i,
                   unsigned int *leading_zeros = NULL) {
  *i = 0;
  bool at_start = true;
  unsigned int zeros = 0;
  while (isdigit(**input)) {
    if (at_start && **input == '0') {
      zeros++;
    } else if (at_start) {
      at_start = false;
    }

    *i *= 10;
    *i += **input - '0';
    (*input)++;
  }
  if (leading_zeros) {
    *leading_zeros = zeros;
  }
  return true;
}

/**
 * Parse a JsonNumber
 */
static bool ParseNumber(const char **input, JsonParserInterface *parser) {
  // The number is in the form <full>.<fractional>e<exponent>
  // full and exponent are signed but we track the sign separate from the
  // value.
  // Only full is required.
  uint64_t full = 0;
  uint64_t fractional = 0;
  int64_t signed_exponent = 0;

  uint32_t leading_fractional_zeros = 0;

  bool has_fractional = false;
  bool has_exponent = false;

  bool is_negative = (**input == '-');
  if (is_negative) {
    (*input)++;
    if (**input == 0) {
      return false;
    }
  }

  if (**input == '0') {
    (*input)++;
  } else if (isdigit(**input)) {
    ExtractDigits(input, &full);
  } else {
    return false;
  }

  if (**input == '.') {
    (*input)++;
    ExtractDigits(input, &fractional, &leading_fractional_zeros);
    has_fractional = true;
  }

  if (**input == 'e' || **input == 'E') {
    bool negative_exponent = false;
    // Handle the exponent
    (*input)++;

    switch (**input) {
      case '-':
        negative_exponent = true;
        // fall through
      case '+':
        (*input)++;
        break;
      default:
        {};
    }

    if (**input == 0) {
      return false;
    }

    uint64_t exponent;
    if (isdigit(**input)) {
      ExtractDigits(input, &exponent);
    } else {
      return false;
    }
    signed_exponent = (negative_exponent ? -1 : 1) * exponent;
    has_exponent = true;
  }

  // Now we have all the components, run the correct callback.
  if (has_fractional || has_exponent) {
    JsonDouble::DoubleRepresentation double_rep;
    double_rep.is_negative = is_negative;
    double_rep.full = full;
    double_rep.leading_fractional_zeros = leading_fractional_zeros;
    double_rep.fractional = fractional;
    double_rep.exponent = signed_exponent;
    parser->Number(double_rep);
    return true;
  }

  if (is_negative) {
    int64_t value = -1 * full;
    if (value < INT32_MIN || value > INT32_MAX) {
      parser->Number(value);
    } else {
      parser->Number(static_cast<int32_t>(value));
    }
  } else {
    if (full > UINT32_MAX) {
      parser->Number(full);
    } else {
      parser->Number(static_cast<uint32_t>(full));
    }
  }
  return true;
}

/**
 * Starts from the first character after the  '['.
 */
static bool ParseArray(const char **input, JsonParserInterface *parser) {
  if (!TrimWhitespace(input)) {
    parser->SetError("Unterminated array");
    return false;
  }

  parser->OpenArray();

  if (**input == ']') {
    (*input)++;
    parser->CloseArray();
    return true;
  }

  while (true) {
    if (!TrimWhitespace(input)) {
      parser->SetError("Unterminated array");
      return false;
    }

    bool result = ParseTrimmedInput(input, parser);
    if (!result) {
      OLA_INFO << "Invalid input";
      return false;
    }

    if (!TrimWhitespace(input)) {
      parser->SetError("Unterminated array");
      return false;
    }

    switch (**input) {
      case ']':
        (*input)++;  // move past the ]
        parser->CloseArray();
        return true;
      case ',':
        break;
      default:
        parser->SetError("Expected either , or ] after an array element");
        return false;
    }
    (*input)++;  // move past the ,
  }
}

/**
 * Starts from the first character after the  '{'.
 */
static bool ParseObject(const char **input, JsonParserInterface *parser) {
  if (!TrimWhitespace(input)) {
    parser->SetError("Unterminated object");
    return false;
  }

  parser->OpenObject();

  if (**input == '}') {
    (*input)++;
    parser->CloseObject();
    return true;
  }

  while (true) {
    if (!TrimWhitespace(input)) {
      parser->SetError("Unterminated object");
      return false;
    }

    if (**input != '"') {
      parser->SetError("Expected key for object");
      OLA_INFO << "Expected string";
      return false;
    }
    (*input)++;

    string key;
    if (!ParseString(input, &key, parser)) {
      return false;
    }
    parser->ObjectKey(key);

    if (!TrimWhitespace(input)) {
      parser->SetError("Missing : after key");
      return false;
    }

    if (**input != ':') {
      parser->SetError("Incorrect character after key, should be :");
      return false;
    }
    (*input)++;

    if (!TrimWhitespace(input)) {
      parser->SetError("Unterminated object");
      return false;
    }

    bool result = ParseTrimmedInput(input, parser);
    if (!result) {
      return false;
    }

    if (!TrimWhitespace(input)) {
      parser->SetError("Unterminated object");
      return false;
    }

    switch (**input) {
      case '}':
        (*input)++;  // move past the }
        parser->CloseObject();
        return true;
      case ',':
        break;
      default:
        parser->SetError("Expected either , or } after an object value");
        return false;
    }
    (*input)++;  // move past the ,
  }
}

static bool ParseTrimmedInput(const char **input,
                             JsonParserInterface *parser) {
  static const char TRUE_STR[] = "true";
  static const char FALSE_STR[] = "false";
  static const char NULL_STR[] = "null";

  if (**input == '"') {
    (*input)++;
    string str;
    if (ParseString(input, &str, parser)) {
      parser->String(str);
      return true;
    }
    return false;
  } else if (strncmp(*input, TRUE_STR, sizeof(TRUE_STR) - 1) == 0) {
    *input += sizeof(TRUE_STR) - 1;
    parser->Bool(true);
    return true;
  } else if (strncmp(*input, FALSE_STR, sizeof(FALSE_STR) - 1) == 0) {
    *input += sizeof(FALSE_STR) - 1;
    parser->Bool(false);
    return true;
  } else if (strncmp(*input, NULL_STR, sizeof(NULL_STR) - 1) == 0) {
    *input += sizeof(NULL_STR) - 1;
    parser->Null();
    return true;
  } else if (**input == '-' || isdigit(**input)) {
    return ParseNumber(input, parser);
  } else if (**input == '[') {
    (*input)++;
    return ParseArray(input, parser);
  } else if (**input == '{') {
    (*input)++;
    return ParseObject(input, parser);
  }
  parser->SetError("Invalid JSON value");
  return false;
}


bool ParseRaw(const char *input, JsonParserInterface *parser) {
  if (!TrimWhitespace(&input)) {
    parser->SetError("No JSON data found");
    return false;
  }

  parser->Begin();
  bool result = ParseTrimmedInput(&input, parser);
  if (!result) {
    return false;
  }
  parser->End();
  return !TrimWhitespace(&input);
}

bool JsonLexer::Parse(const string &input,
                      JsonParserInterface *parser) {
  // TODO(simon): Do we need to convert to unicode here? I think this may be
  // an issue on Windows. Consider mbstowcs.
  // Copying the input sucks though, so we should use input.c_str() if we can.
  char* input_data = new char[input.size() + 1];
  strncpy(input_data, input.c_str(), input.size() + 1);

  bool result = ParseRaw(input_data, parser);
  delete[] input_data;
  return result;
}
}  // namespace web
}  // namespace ola
