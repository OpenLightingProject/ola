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
 * PathportCommon.h
 * Constants for the shownet plugin
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef COMMON_H
#define COMMON_H

namespace lla {
namespace plugin {
namespace pathport {

// number of pathport ports we have per device
// this is split between input and ouput ports
static const unsigned int PORTS_PER_DEVICE = 8;

} // pathport
} // plugin
} // lla

#endif
