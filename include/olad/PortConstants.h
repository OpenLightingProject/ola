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
 * PortConstants.h
 * Header file for the Port Constants.
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLAD_PORT_CONSTANTS_H_
#define INCLUDE_OLAD_PORT_CONSTANTS_H_

namespace ola {
  typedef enum {
    PRIORITY_MODE_INHERIT,
    PRIORITY_MODE_OVERRIDE,
    PRIORITY_MODE_END,
  } port_priority_mode;

  typedef enum {
    CAPABILITY_NONE,  // port doesn't support priorities at all
    CAPABILITY_STATIC,  // port allows a static priority assignment
    CAPABILITY_FULL,  // port can either inherit or use a static assignment
  } port_priority_capability;
}  // ola
#endif  // INCLUDE_OLAD_PORT_H_
