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
 * RDMPDU.h
 * The RDMPDU class
 * Copyright (C) 2012 Simon Newton
 */

#ifndef LIBS_ACN_RDMPDU_H_
#define LIBS_ACN_RDMPDU_H_

#include <ola/io/IOStack.h>

#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

class RDMPDU : private PDU {
 public:
  static void PrependPDU(ola::io::IOStack *stack);
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_RDMPDU_H_
