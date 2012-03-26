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
 * E133Endpoint.h
 * Copyright (C) 2012 Simon Newton
 */

#include "ola/rdm/RDMControllerInterface.h"

#ifndef TOOLS_E133_E133ENDPOINT_H_
#define TOOLS_E133_E133ENDPOINT_H_

/**
 * An E1.33 Endpoint which can be registered with the E133Device.
 * Endpoints are tasked with handling E1.33 / RDM requests.
 */
class E133Endpoint: public ola::rdm::RDMControllerInterface {
  public:
    E133Endpoint() {}
    virtual ~E133Endpoint() {}
};
#endif  // TOOLS_E133_E133ENDPOINT_H_
