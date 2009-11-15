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
 * RunLengthEncoder.h
 * Header file for the RunLengthEncoder class
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef INCLUDE_OLA_RUNLENGTHENCODER_H_
#define INCLUDE_OLA_RUNLENGTHENCODER_H_

#include <ola/DmxBuffer.h>

namespace ola {

class RunLengthEncoder {
  public :
    RunLengthEncoder() {}
    ~RunLengthEncoder() {}

    bool Encode(const DmxBuffer &src,
                uint8_t *data,
                unsigned int &size);
    bool Decode(DmxBuffer *dst,
                unsigned int start_channel,
                const uint8_t *data,
                unsigned int length);
  private:
    static const uint8_t REPEAT_FLAG = 0x80;
};

}  // ola
#endif  // INCLUDE_OLA_RUNLENGTHENCODER_H_
