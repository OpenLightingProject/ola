/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Random.h
 * A simple random number generator.
 * Copyright (C) 2012 Simon Newton
 */

#ifndef INCLUDE_OLA_MATH_RANDOM_H_
#define INCLUDE_OLA_MATH_RANDOM_H_

#include <ola/io/InputBuffer.h>
#include <ola/io/OutputBuffer.h>
#include <stdint.h>
#include <sys/uio.h>
#include <deque>
#include <iostream>
#include <queue>
#include <string>

namespace ola {
namespace math {

void InitRandom();
int Random(int lower, int upper);
}  // math
}  // ola
#endif  // INCLUDE_OLA_MATH_RANDOM_H_
