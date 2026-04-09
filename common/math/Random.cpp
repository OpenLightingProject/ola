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
 * Random.cpp
 * Random number generator
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_RANDOM
#include <random>
#endif  // HAVE_RANDOM

#include "ola/Clock.h"
#include "ola/math/Random.h"

namespace ola {
namespace math {

#ifdef HAVE_RANDOM
std::default_random_engine generator_;
#endif  // HAVE_RANDOM

void InitRandom() {
  Clock clock;
  TimeStamp now;
  // The clock type here should not matter because only the microseconds field
  // is being used to seed the random number generator.
  clock.CurrentRealTime(&now);

  uint64_t seed = (static_cast<uint64_t>(now.MicroSeconds()) << 32) +
                   static_cast<uint64_t>(getpid());
#ifdef HAVE_RANDOM
  generator_.seed(seed);
#elif defined(_WIN32)
  srand(seed);
#else
  srandom(seed);
#endif  // HAVE_RANDOM
}

int Random(int lower, int upper) {
#ifdef HAVE_RANDOM
  std::uniform_int_distribution<int> distribution(lower, upper);
  return distribution(generator_);
#elif defined(_WIN32)
  return (lower +
          (rand() % (upper - lower + 1)));  // NOLINT(runtime/threadsafe_fn)
#else
  return (lower + (random() % (upper - lower + 1)));
#endif  // HAVE_RANDOM
}
}  // namespace math
}  // namespace ola
