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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * DmxSource.cpp
 * The DmxSource class.
 * Copyright (C) 2005-2010 Simon Newton
 */

#include <olad/DmxSource.h>

namespace ola {

const uint8_t DmxSource::PRIORITY_MIN = 0;
const uint8_t DmxSource::PRIORITY_MAX = 200;
const uint8_t DmxSource::PRIORITY_DEFAULT = 100;

const TimeInterval DmxSource::TIMEOUT_INTERVAL(2500000);  // 2.5s
}  // ola
