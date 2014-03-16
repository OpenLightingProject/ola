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
 * JsonParser.h
 * A class for Parsing Json data.
 * See http://www.json.org/
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonParser.h
 * @brief Header file for the JSON parser.
 * The implementation does it's best to conform to
 * http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONPARSER_H_
#define INCLUDE_OLA_WEB_JSONPARSER_H_

#include <ola/web/JsonHandler.h>
#include <string>

namespace ola {
namespace web {

/**
 * @addtogroup json
 * @{
 */

/**
 * @brief Parse a string containing Json data.
 */
class JsonParser {
 public:
  /**
   * @brief Parse a string with json data
   * @param input the input string
   * @param handler the JsonHandlerInterface to pass tokens to.
   * @return true if parsing was successfull, false otherwise.
   */
  static bool Parse(const std::string &input,
                    JsonHandlerInterface *handler);
};

/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONPARSER_H_
