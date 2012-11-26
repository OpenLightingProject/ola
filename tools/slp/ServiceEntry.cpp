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
 * ServiceEntry.cpp
 * Copyright (C) 2012 Simon Newton
 */

#include <vector>
#include "tools/slp/ServiceEntry.h"
#include "tools/slp/SLPPacketBuilder.h"

namespace ola {
namespace slp {

void URLEntry::Write(ola::io::BigEndianOutputStreamInterface *output) const {
  *output << static_cast<uint8_t>(0);  // reservered
  *output << m_lifetime;
  SLPPacketBuilder::WriteString(output, m_url);
  *output << static_cast<uint8_t>(0);  // # of URL auths
}


/**
 * Mark as having registered with a DA or update the existing entry.
 */
void LocalServiceEntry::UpdateDA(const IPV4Address &address,
                                 const TimeStamp &expires_in) {
  m_registered_das[address] = expires_in;
}

/**
 * Mark as having been de-registered from a DA
 */
void LocalServiceEntry::RemoveDA(const IPV4Address &address) {
  m_registered_das.erase(address);
}


/**
 * Return the DAs this service is registered with. This clears the output.
 */
void LocalServiceEntry::RegisteredDAs(vector<IPV4Address> *output) const {
  output->clear();
  output->reserve(m_registered_das.size());
  for (DATimeMap::const_iterator iter = m_registered_das.begin();
       iter != m_registered_das.end(); ++iter)
    output->push_back(iter->first);
}


/**
 * Return the list of registrations that will expire before the limit
 */
void LocalServiceEntry::OldRegistrations(const TimeStamp &limit,
                                         vector<IPV4Address> *output) const {
  for (DATimeMap::const_iterator iter = m_registered_das.begin();
       iter != m_registered_das.end(); ++iter)
    if (iter->second < limit)
      output->push_back(iter->first);
}


/**
 * Set the new lifetime for this service
 */
void LocalServiceEntry::SetLifetime(uint16_t lifetime, const TimeStamp &now) {
  m_service.mutable_url().set_lifetime(lifetime);
  m_expires_at = now + TimeInterval(lifetime, 0);
}


void LocalServiceEntry::ToStream(ostream &out) const {
  m_service.ToStream(out);
  vector<IPV4Address> das;
  RegisteredDAs(&das);
  out << ", Reg with: " << ola::StringJoin(",", das);
}
}  // slp
}  // ola
