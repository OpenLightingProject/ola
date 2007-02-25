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
 * Copyright (C) 2006-2007 Simon Newton
 */

#ifndef LLA_CLIENT_OBSERVER_H
#define LLA_CLIENT_OBSERVER_H

using namespace std;

#include <stdint.h>
#include <vector>

class LlaClientObserver {

  public:
    virtual ~LlaClientObserver() {}

    virtual int new_dmx(unsigned int uni, unsigned int length, uint8_t *data) { return 0; }
    virtual int universes(const vector <class LlaUniverse *> unis) { return 0; }
    virtual int plugins(const vector <class LlaPlugin *> plugins) { return 0; }
    virtual int devices(const vector <class LlaDevice *> devices) { return 0; }
    virtual int ports(class LlaDevice *dev) { return 0; }
    virtual int plugin_desc(class LlaPlugin *plug) { return 0; }
    virtual int dev_config(unsigned int dev, uint8_t *req, unsigned int len) { return 0; }

  private:

};
#endif
