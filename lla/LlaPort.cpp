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
 * LlaPort.cpp
 * Implementation of the LLA Client Port class
 * Copyright (C) 2005-2006 Simon Newton
 */

#include "LlaPort.h"

/*
 *
 *
 */
LlaPort::LlaPort(int id, PortCapability cap, int uni, int active) :
	m_id(id),
	m_cap(cap),
	m_uni(uni),
	m_active(active) {

}
