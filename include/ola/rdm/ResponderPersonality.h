/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * ResponderPersonality.h
 * Manages personalities for a RDM responder.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERPERSONALITY_H_
#define INCLUDE_OLA_RDM_RESPONDERPERSONALITY_H_

#include <ola/Logging.h>
#include <ola/base/Macro.h>
#include <ola/rdm/ResponderSlotData.h>

#include <stdint.h>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * Represents a personality.
 */
class Personality {
 public:
    Personality(uint16_t footprint, const std::string &description);
    Personality(uint16_t footprint, const std::string &description,
                const SlotDataCollection &slot_data);

    uint16_t Footprint() const { return m_footprint; }
    std::string Description() const { return m_description; }

    const SlotDataCollection* GetSlotData() const { return &m_slot_data; }

    const SlotData *GetSlotData(uint16_t slot_number) const {
      return m_slot_data.Lookup(slot_number);
    }

 private:
    uint16_t m_footprint;
    std::string m_description;
    SlotDataCollection m_slot_data;
};


/**
 * Holds the list of personalities for a class of responder. A single instance
 * is shared between all responders of the same type. Subclass this and use a
 * singleton.
 */
class PersonalityCollection {
 public:
    /** The data type that stores the list of personalities for a responder. */
    typedef std::vector<Personality> PersonalityList;

    explicit PersonalityCollection(const PersonalityList &personalities);
    virtual ~PersonalityCollection();

    uint8_t PersonalityCount() const;

    const Personality *Lookup(uint8_t personality) const;

 protected:
    PersonalityCollection() {}

 private:
    const PersonalityList m_personalities;

    DISALLOW_COPY_AND_ASSIGN(PersonalityCollection);
};


/**
 * Manages the personalities for a single responder
 */
class PersonalityManager {
 public:
    PersonalityManager() : m_active_personality(0) {}
    explicit PersonalityManager(const PersonalityCollection *personalities);

    uint8_t PersonalityCount() const;
    bool SetActivePersonality(uint8_t personality);
    uint8_t ActivePersonalityNumber() const { return m_active_personality; }
    const Personality *ActivePersonality() const;
    uint16_t ActivePersonalityFootprint() const;
    std::string ActivePersonalityDescription() const;
    const Personality *Lookup(uint8_t personality) const;

 private:
    const PersonalityCollection *m_personalities;
    uint8_t m_active_personality;

    DISALLOW_COPY_AND_ASSIGN(PersonalityManager);
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERPERSONALITY_H_
