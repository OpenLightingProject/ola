/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * ResponderPersonality.h
 * Manages personalities for a RDM responder.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERPERSONALITY_H_
#define INCLUDE_OLA_RDM_RESPONDERPERSONALITY_H_

#include <string>
#include <vector>

namespace ola {
namespace rdm {

using std::string;
using std::vector;

/**
 * Represents a personality.
 */
class Personality {
  public:
    Personality(uint16_t footprint, const string &description);

    uint16_t Footprint() const { return m_footprint; }
    string Description() const { return m_description; }

  private:
    uint16_t m_footprint;
    const string m_description;
};


/**
 * Holds the list of personalities for a class of responder. A single instance
 * is shared between all responders of the same type. Subclass this and use a
 * singleton.
 */
class PersonalityCollection {
  public:
    explicit PersonalityCollection(const vector<Personality*> &personalities);
    virtual ~PersonalityCollection();

    uint8_t PersonalityCount() const;

    const Personality *Lookup(uint8_t personality) const;

  protected:
    PersonalityCollection() {}

  private:
    std::vector<Personality*> m_personalities;
};


/**
 * Manages the personalities for a single responder
 */
class PersonalityManager {
  public:
    explicit PersonalityManager(const PersonalityCollection *personalities);

    uint8_t PersonalityCount() const;
    bool SetActivePersonality(uint8_t personality);
    uint8_t ActivePersonalityNumber() const { return m_active_personality; }
    const Personality *ActivePersonality() const;
    uint16_t ActivePersonalityFootprint() const;
    const Personality *Lookup(uint8_t personality) const;

  private:
    const PersonalityCollection *m_personalities;
    uint8_t m_active_personality;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERPERSONALITY_H_
