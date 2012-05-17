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
 * DummyResponder.cpp
 * The Dummy Responder for ola
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include "ola/BaseTypes.h"
#include "ola/Clock.h"
#include "ola/Logging.h"
#include "ola/rdm/RDMEnums.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/dummy/DummyResponder.h"

namespace ola {
namespace plugin {
namespace dummy {

using ola::network::HostToNetwork;
using ola::network::NetworkToHost;
using ola::rdm::GetResponseFromData;
using ola::rdm::NackWithReason;
using ola::rdm::RDMRequest;
using ola::rdm::RDMResponse;
using std::string;
using std::vector;


const DummyResponder::personality_info DummyResponder::PERSONALITIES[] = {
  {5, "Personality 1"},
  {10, "Personality 2"},
  {20, "Personality 3"},
};

const unsigned int DummyResponder::PERSONALITY_COUNT = (
  sizeof(DummyResponder::PERSONALITIES) /
  sizeof(DummyResponder::personality_info));


/*
 * Handle an RDM Request
 */
void DummyResponder::SendRDMRequest(const ola::rdm::RDMRequest *request,
                                    ola::rdm::RDMCallback *callback) {
  m_root_device.SendRDMRequest(request, callback);
}


}  // dummy
}  // plugin
}  // ola
