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
 * TokenBucket.h
 * Header file for the token bucket class
 * Copyright (C) 2010 Simon Newton
 */

#ifndef INCLUDE_OLAD_TOKENBUCKET_H_
#define INCLUDE_OLAD_TOKENBUCKET_H_

#include <ola/Clock.h>

namespace ola {

class TokenBucket {
  public:
    TokenBucket(unsigned int initial,
                unsigned int rate_per_second,
                unsigned int max,
                const TimeStamp &now):
        m_count(initial),
        m_rate(rate_per_second),
        m_max(max),
        m_last(now) {
    }

    bool GetToken(const TimeStamp &now);
    unsigned int Count(const TimeStamp &now);

  private:
    unsigned int m_count;
    unsigned int m_rate;
    unsigned int m_max;
    TimeStamp m_last;

    TokenBucket(const TokenBucket&);
    TokenBucket& operator=(const TokenBucket&);
};
}  // ola
#endif  // INCLUDE_OLAD_TOKENBUCKET_H_
