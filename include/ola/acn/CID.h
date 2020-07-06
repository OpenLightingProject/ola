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
 * CID.h
 * The CID class, this just wraps a CIDImpl so we don't need to include all the
 *   UUID headers.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef INCLUDE_OLA_ACN_CID_H_
#define INCLUDE_OLA_ACN_CID_H_

/**
 * @addtogroup acn
 * @{
 * @file CID.h
 * @brief The ACN component identifier.
 * @}
 */

#include <stdint.h>
#include <ola/io/OutputBuffer.h>
#include <string>

namespace ola {
namespace acn {

/**
 * @addtogroup acn
 * @{
 * @class CID
 * @brief The ACN component identifier.
 * @}
 */
class CID {
 public :
  /**
   * @brief The length of the CID data.
   */
  enum {
    CID_LENGTH = 16  /**< The length of a CID in binary form */
  };

  /**
   * @brief Create a new uninitialized CID.
   */
  CID();

  /**
   * @brief Copy constructor
   */
  CID(const CID& other);

  /**
   * @brief CID destructor.
   */
  ~CID();

  /**
   * @brief Returns true if the CID is uninitialized.
   */
  bool IsNil() const;

  /**
   * @brief Pack a CID into the binary representation.
   * @param[out] output the data buffer to use for the CID. Must be at least
   * CID_LENGTH long.
   */
  void Pack(uint8_t *output) const;

  /**
   * @brief Return the CID as a human readable string.
   * @examplepara
   * @code
   * "D5D46622-ECCB-410D-BC9A-267C6099C136"
   * @endcode
   */
  std::string ToString() const;

  /**
   * @brief Write the CID to an OutputBufferInterface
   */
  void Write(ola::io::OutputBufferInterface *output) const;

  /**
   * @brief Assignment operator
   */
  CID& operator=(const CID& c1);

  /**
   * @brief Equality operator.
   */
  bool operator==(const CID& c1) const;

  /**
   * @brief Inequality operator.
   */
  bool operator!=(const CID& c1) const;

  /**
   * @brief Less than operator.
   */
  bool operator<(const CID& c1) const;

  /**
   * @brief Generate a new CID
   */
  static CID Generate();

  /**
   * @brief Create a new CID from a binary representation
   * @param data the memory location that stores the binary representation of
   * the CID.
   */
  static CID FromData(const uint8_t *data);

  /**
   * @brief Create a new CID from a human readable string
   * @param cid the CID in string format.
   */
  static CID FromString(const std::string &cid);

 private:
  class CIDImpl *m_impl;

  // Takes ownership;
  explicit CID(class CIDImpl *impl);
};
}  // namespace acn
}  // namespace ola

/**
 * @}
 */
#endif  // INCLUDE_OLA_ACN_CID_H_
