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
 * E133StatusPDU.h
 * The E133StatusPDU
 * Copyright (C) 2013 Simon Newton
 */

#ifndef LIBS_ACN_E133STATUSPDU_H_
#define LIBS_ACN_E133STATUSPDU_H_

#include <ola/io/IOStack.h>
#include <string>
#include "ola/e133/E133Enums.h"
#include "libs/acn/PDU.h"

namespace ola {
namespace acn {

class E133StatusPDU : private PDU {
 public:
    static void PrependPDU(ola::io::IOStack *stack,
                           ola::e133::E133StatusCode status_code,
                           const std::string &status);
};
}  // namespace acn
}  // namespace ola
#endif  // LIBS_ACN_E133STATUSPDU_H_
