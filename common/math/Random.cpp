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
 * Random.cpp
 * Random number generator
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <time.h>

#ifdef HAVE_RANDOM
#include <random>
#endif

#include "ola/math/Random.h"

namespace ola {
namespace math {


#ifdef HAVE_RANDOM
std::default_random_engine generator_;
#endif

/**
 * Seed the random number generator
 */
void InitRandom() {
#ifdef HAVE_RANDOM
  generator_.seed(time(NULL));
#else
  srandom(time(NULL));
#endif
}


/**
 * Return a random number between lower and upper, inclusive. i.e.
 * [lower, upper]
 */
int Random(int lower, int upper) {
#ifdef HAVE_RANDOM
  std::uniform_int_distribution<int> distribution(lower, upper);
  return distribution(generator_);
#else
  return lower + (random() % (upper - lower + 1));
#endif
}
}  // namespace math
}  // namespace ola
