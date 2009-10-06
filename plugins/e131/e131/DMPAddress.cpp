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
 * DMPAddress.cpp
 * Defines the DMP property address types
 * Copyright (C) 2007 Simon Newton
 */

#include "DMPAddress.h"

namespace ola {
namespace e131 {


/*
 * Create a new single address
 */
DMPAddress *NewSingleAddress(unsigned int value) {
  if (value > 0xffff)
    return new FourByteSingleDMPAddress(value);
  else if (value > 0xff)
    return new TwoByteSingleDMPAddress(value);
  return new OneByteSingleDMPAddress(value);
}


/*
 * Create a new repeated address.
 */
DMPAddress *NewRepeatedAddress(unsigned int value,
                               unsigned int increment,
                               unsigned int number) {
  if (value > 0xffff)
    return new FourByteRepeatedDMPAddress(value, increment, number);
  else if (value > 0xff)
    return new TwoByteRepeatedDMPAddress(value, increment, number);
  return new OneByteRepeatedDMPAddress(value, increment, number);
}


} // e131
} // ola
