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
 * RunLengthDecoder.h
 * Header file for the RunLengthDecoder class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef ESPNET_RLDECODER_H
#define ESPNET_RLDECODER_H

#include <ola/DmxBuffer.h>

namespace ola {
namespace plugin {
namespace espnet {

class RunLengthDecoder {
  public :
    RunLengthDecoder() {};
    ~RunLengthDecoder() {};

    bool Decode(DmxBuffer &dst,
                const uint8_t *data,
                unsigned int length);
  private:
    static const uint8_t ESCAPE_VALUE = 0xFD;
    static const uint8_t REPEAT_VALUE = 0xFE;
};

} //espnet
} //plugin
} //ola
#endif
