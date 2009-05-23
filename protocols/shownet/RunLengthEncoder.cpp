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
 * RunLengthEncoder.cpp
 * The Run Length Encoder
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <RunLengthEncoder.h>

namespace lla {
namespace shownet {

bool RunLengthEncoder::Encode(const DmxBuffer &src) {


}


bool RunLengthEncoder::Decode(uint8_t *src, int slen, DmxBuffer &dest) {
  int i,l;
  int lr = dlen;
  int di = 0;

  for (i=0; i < slen && lr > 0; ) {
    l = src[i] & (~REPEAT_FLAG);
    l = min(lr, l);

    if (src[i] & REPEAT_FLAG) {
      i++;
      memset(&dst[di], src[i++], min(l, lr));
      di += l;
    } else {
      memcpy(&dst[di], &src[++i], min(l, lr));
      di += l;
      i += l;
    }
    lr -= l;
  }
  return di;
}

} //shownet
} //lla
