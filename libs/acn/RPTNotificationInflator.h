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
 * RPTNotificationInflator.h
 * Interface for the RPTNotificationInflator class.
 * Copyright (C) 2023 Peter Newman
 */

#ifndef LIBS_ACN_RPTNOTIFICATIONINFLATOR_H_
#define LIBS_ACN_RPTNOTIFICATIONINFLATOR_H_

#include "ola/acn/ACNVectors.h"
#include "libs/acn/BaseInflator.h"

namespace ola {
namespace acn {

class RPTNotificationInflator: public BaseInflator {
  friend class RPTNotificationInflatorTest;

 public:
  RPTNotificationInflator()
    : BaseInflator() {
  }
  ~RPTNotificationInflator() {}

  uint32_t Id() const { return ola::acn::VECTOR_RPT_NOTIFICATION; }

 protected:
  // The 'header' is 0 bytes in length.
  bool DecodeHeader(HeaderSet*,
                    const uint8_t*,
                    unsigned int,
                    unsigned int *bytes_used) {
    *bytes_used = 0;
    return true;
  }

  void ResetHeaderField() {}  // namespace noop
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_RPTNOTIFICATIONINFLATOR_H_
