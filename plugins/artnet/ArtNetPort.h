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
 * ArtnetPort.h
 * The Art-Net plugin for lla
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef ARTNETPORT_H
#define ARTNETPORT_H

#include <llad/Port.h>
#include <artnet/artnet.h>

namespace lla {
namespace plugin {

class ArtNetPort: public Port {
  public:
    ArtNetPort(AbstractDevice *parent, int id) : Port(parent, id) {};

    int SetUniverse(Universe *uni) ;
    int WriteDMX(uint8_t *data, unsigned int length);
    int ReadDMX(uint8_t *data, unsigned int length);
    bool CanRead() const;
    bool CanWrite() const ;
};

} //plugin
} //lla

#endif
