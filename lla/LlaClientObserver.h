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
 * LlaClientObserver.h
 * The interface for the LLA Connection Observer class
 * Copyright (C) 2006  Simon Newton
 */

#ifndef LLACONOBSERVER_H
#define LLACONOBSERVER_H

using namespace std;

#include <stdint.h>
#include <vector>

class LlaClientObserver {

	public:

		virtual ~LlaClientObserver() {};
		// devices, uni, plugin

		virtual int new_dmx(int uni,int length, uint8_t *data) = 0;
		virtual int universes(const vector <class LlaUniverse *> unis) = 0;
		virtual int plugins(const vector <class LlaPlugin *> plugins) = 0;
		virtual int devices(const vector <class LlaDevice *> devices) = 0;
		virtual int ports(class LlaDevice *dev) =0;
		virtual int plugin_desc(class LlaPlugin *plug) = 0;

	private:

};
#endif
