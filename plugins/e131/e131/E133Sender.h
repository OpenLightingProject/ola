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
 * E133Sender.h
 * Interface for the E133Sender class, this abstracts the encapsulation and
 * sending of DMP PDUs contained within E133PDUs as well as the setting of
 * the DMP inflator inflator.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_E131_E131_E133LAYER_H_
#define PLUGINS_E131_E131_E133LAYER_H_

#include "ola/network/IPV4Address.h"
#include "plugins/e131/e131/ACNPort.h"
#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133Inflator.h"
#include "plugins/e131/e131/Transport.h"

namespace ola {
namespace plugin {
namespace e131 {

class E133Sender {
  public:
    explicit E133Sender(class RootSender *root_sender);
    ~E133Sender() {}

    bool SendDMP(const E133Header &header,
                 const class DMPPDU *pdu,
                 OutgoingTransport *transport);
    bool SetInflator(class DMPE133Inflator *inflator);

  private:
    class RootSender *m_root_sender;
    E133Inflator m_e133_inflator;

    E133Sender(const E133Sender&);
    E133Sender& operator=(const E133Sender&);
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_E133LAYER_H_
