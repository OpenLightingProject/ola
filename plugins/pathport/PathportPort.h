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
 *
 * pathportport.h
 * The Pathport plugin for lla
 * Copyright (C) 2005-2007 Simon Newton
 */

#ifndef PATHPORTPORT_H
#define PATHPORTPORT_H

#include <llad/port.h>

#include <pathport/pathport.h>

class PathportPort : public Port {

	public:
		PathportPort(Device *parent, int id);
		~PathportPort();
		
		int write(uint8_t *data, int length);
		int read(uint8_t *data, int length); 
		
		int can_read() const;
		int can_write() const;
		
		int update_buffer(const uint8_t *data, int length);
		int set_universe(Universe *uni);

	private :
		uint8_t *m_buf;
		int m_len;
};

#endif
