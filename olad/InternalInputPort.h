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
 * InternalInputPort.h
 * An input port used to send RDM commands to a universe
 * Copyright (C) 2010 Simon Newton
 */

#ifndef OLAD_INTERNALINPUTPORT_H_
#define OLAD_INTERNALINPUTPORT_H_

#include <string>
#include "ola/Logging.h"
#include "ola/rdm/RDMCommand.h"
#include "olad/Port.h"

namespace ola {

/*
 * This handles RDM Responses
 */
class InternalInputPortResponseHandler {
  public:
    virtual ~InternalInputPortResponseHandler() {}

    virtual bool HandleRDMResponse(unsigned int universe,
                                   const ola::rdm::RDMResponse *response) = 0;
};


/*
 * This class is a special type of Input port, used to send RDM commands
 * generated interally.
 */
class InternalInputPort: public BasicInputPort {
  public:
    InternalInputPort(unsigned int port_id,
                      InternalInputPortResponseHandler *handler):
      BasicInputPort(NULL, port_id, NULL),
      m_handler(handler) {}

    const DmxBuffer &ReadDMX() const;
    bool HandleRDMResponse(const ola::rdm::RDMResponse *response);
    string UniqueId() const;
    string Description() const { return "Internal Port"; }

  private:
    DmxBuffer m_buffer;
    InternalInputPort(const InternalInputPort&);
    InternalInputPort& operator=(const InternalInputPort&);
    InternalInputPortResponseHandler *m_handler;
};
}  // ola
#endif  // OLAD_INTERNALINPUTPORT_H_
