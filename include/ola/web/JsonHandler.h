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
 * JsonHandler.h
 * The interface for JsonHandlers.
 * See http://www.json.org/
 * Copyright (C) 2014 Simon Newton
 */

/**
 * @addtogroup json
 * @{
 * @file JsonHandler.h
 * @brief Header file for the JSON parser.
 * The implementation does it's best to conform to
 * http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf
 * @}
 */

#ifndef INCLUDE_OLA_WEB_JSONHANDLER_H_
#define INCLUDE_OLA_WEB_JSONHANDLER_H_

#include <stdint.h>
#include <ola/base/Macro.h>
#include <string>

namespace ola {
namespace web {

class JsonValue;

/**
 * @addtogroup json
 * @{
 */

/**
 * @brief The interface for Json Handlers.
 *
 * As the JsonParser traverses the input, it calls the methods below.
 */
class JsonHandlerInterface {
 public:
  JsonHandlerInterface() {}
  virtual ~JsonHandlerInterface() {}

  /**
   * @brief Called when parsing begins.
   */
  virtual void Begin() = 0;

  /**
   * @brief Called when parsing completes.
   */
  virtual void End() = 0;

  /**
   * @brief Called when a string is encounted.
   *
   * This is not called for object keys, see ObjectKey() below.
   */
  virtual void String(const std::string &value) = 0;

  virtual void Number(uint32_t value) = 0;
  virtual void Number(int32_t value) = 0;
  virtual void Number(uint64_t value) = 0;
  virtual void Number(int64_t value) = 0;

  // MinGW struggles with long doubles
  // http://mingw.5.n7.nabble.com/Strange-behaviour-of-gcc-4-8-1-with-long-double-td32949.html
  // To avoid this, and to keep as many significant bits as possible we keep
  // the components separate. See JsonDoubleValue for details.
  virtual void Number(bool is_negative, uint64_t full,
                      int32_t leading_fractional_zeros, uint64_t fractional,
                      int32_t exponent) = 0;

  /**
   * @brief Called when a bool is encounted.
   */
  virtual void Bool(bool value) = 0;

  /**
   * @brief Called when a null token is encounted.
   */
  virtual void Null() = 0;

  /**
   * @brief Called when an array starts.
   */
  virtual void OpenArray() = 0;

  /**
   * @brief Called when an array completes.
   */
  virtual void CloseArray() = 0;

  /**
   * @brief Called when an object starts.
   */
  virtual void OpenObject() = 0;

  /**
   * @brief Called when a new key is encounted.
   *
   * This may be called multiple times for the same object. The standard
   * doesn't specify how to handle duplicate keys, so I generally use the last
   * one.
   */
  virtual void ObjectKey(const std::string &key) = 0;

  /**
   * @brief Called when an object completes.
   */
  virtual void CloseObject() = 0;

  /**
   * @brief Can be called at any time to indicate an error with the input data.
   */
  virtual void SetError(const std::string &error) = 0;
};


/**
 * @brief A Handler that does nothing but verify the syntax.
 *
 * The parsed data is discarded.
 * This is a convinient class to inherit from when writing your own handler.
 */
class NullHandler : public JsonHandlerInterface {
 public:
  NullHandler() : JsonHandlerInterface() {}
  virtual ~NullHandler() {}

  virtual void Begin() {}
  virtual void End() {}

  virtual void String(const std::string &) {}
  virtual void Number(uint32_t) {}
  virtual void Number(int32_t) {}
  virtual void Number(uint64_t) {}
  virtual void Number(int64_t) {}
  virtual void Number(bool, uint64_t, int32_t, uint64_t, int32_t) {}
  virtual void Bool(bool value) { (void) value; }
  virtual void Null() {}
  virtual void OpenArray() {}
  virtual void CloseArray() {}
  virtual void OpenObject() {}
  virtual void CloseObject() {}

  virtual void SetError(const std::string &error) { m_error = error; }

  /**
   * @brief Check if parsing was sucessfull.
   */
  std::string GetError() const { return m_error; }

 private:
  std::string m_error;

  DISALLOW_COPY_AND_ASSIGN(NullHandler);
};
/**@}*/
}  // namespace web
}  // namespace ola
#endif  // INCLUDE_OLA_WEB_JSONHANDLER_H_
