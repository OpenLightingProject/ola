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
 * E133Component.h
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Clock.h>
#include <string>

#include "plugins/e131/e131/E133Header.h"
#include "plugins/e131/e131/E133Layer.h"
#include "plugins/e131/e131/TransportHeader.h"

#ifndef TOOLS_E133_E133COMPONENT_H_
#define TOOLS_E133_E133COMPONENT_H_


/**
 * An E1.33 Component which can be registered with the E133Node.
 */
class E133Component {
  public:
    E133Component() {}
    virtual ~E133Component() {}

    virtual unsigned int Universe() const = 0;
    virtual void SetE133Layer(ola::plugin::e131::E133Layer *e133_layer) = 0;

    virtual void HandleResponse(
        const ola::plugin::e131::TransportHeader &transport_header,
        const ola::plugin::e131::E133Header &e133_header,
        const std::string &raw_response) = 0;

    virtual void CheckForStaleRequests(const ola::TimeStamp *now) = 0;
};
#endif  // TOOLS_E133_E133COMPONENT_H_
