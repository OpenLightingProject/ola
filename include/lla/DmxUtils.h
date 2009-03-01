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
 * DmxUtil..h
 * Random Dmx functions.
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef LLA_DMX_UTILS_H
#define LLA DMX_UTILS_H

#include <string>
#include <lla/BaseTypes.h>

namespace lla {

unsigned int StringToDmx(const std::string &input, dmx_t *dmx_data,
                         unsigned int length);

} //lla

#endif
