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
 * E131Layer.h
 * Interface for the E131Layer class, this abstracts the encapsulation and
 * sending of DMP PDUs contained within E131PDUs as well as the setting of
 * the DMP inflator inflator.
 * Copyright (C) 2007 Simon Newton
 */

#ifndef OLA_E131_E131LAYER_H
#define OLA_E131_E131LAYER_H

#include "E131Header.h"
#include "E131PDU.h"
#include "E131Inflator.h"
#include "RootLayer.h"

namespace ola {
namespace e131 {

class DMPInflator;

class E131Layer {
  public:
    E131Layer(RootLayer *root_layer);
    ~E131Layer() {}

    //bool SetDmpInflator(BaseInflator *inflator);

    //bool SendDmp(const E131Header &header, const DMPPDU &pdu);

    bool SetInflator(DMPInflator *inflator);
    bool JoinUniverse(unsigned int universe);
    bool LeaveUniverse(unsigned int universe);

  private:
    RootLayer *m_root_layer;
    E131Inflator m_e131_inflator;

    E131Layer(const E131Layer&);
    E131Layer& operator=(const E131Layer&);
    bool UniverseIP(unsigned int universe, struct in_addr &addr);
};

} //e131
} //ola

#endif
