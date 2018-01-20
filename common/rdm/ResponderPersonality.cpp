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
 * ResponderPersonality.cpp
 * Manages personalities for a RDM responder.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef COMMON_RDM_RESPONDERPERSONALITY_H_
#define COMMON_RDM_RESPONDERPERSONALITY_H_

#include <string>

#include "ola/rdm/ResponderPersonality.h"
#include "ola/rdm/ResponderSlotData.h"
#include "ola/stl/STLUtils.h"

namespace ola {
namespace rdm {

using std::string;

/**
 * Create a new personality.
 * @param footprint the number of dmx slots consumed
 * @param description the personality name (32 chars)
 */
Personality::Personality(uint16_t footprint, const string &description)
    : m_footprint(footprint),
      m_description(description) {
        // STATIC_ASSERT(description.length() <= 32);
        // c++11:
        // static_assert(
        //   sizeof(description) <= 32,
        //   "Personality Description is to long (must be <= 32 chars)");
}

Personality::Personality(uint16_t footprint, const string &description,
                         const SlotDataCollection &slot_data)
    : m_footprint(footprint),
      m_description(description),
      m_slot_data(slot_data) {
        // constexpr int descriptionlen = description.length();
        // STATIC_ASSERT(descriptionlen <= 32);
        // const int descriptionlen = description.length();
}

/**
 * Takes ownership of the personalites
 */
PersonalityCollection::PersonalityCollection(
    const PersonalityList &personalities)
    : m_personalities(personalities) {
}

/**
 * Clean up
 */
PersonalityCollection::~PersonalityCollection() {
}

/**
 * @returns the number of personalities
 */
uint8_t PersonalityCollection::PersonalityCount() const {
  return m_personalities.size();
}

/**
 * Look up a personality by index
 */
const Personality *PersonalityCollection::Lookup(uint8_t personality) const {
  if (personality == 0 || personality > m_personalities.size())
    return NULL;
  return &m_personalities[personality - 1];
}

PersonalityManager::PersonalityManager(
    const PersonalityCollection *personalities)
    : m_personalities(personalities),
      m_active_personality(1) {
}

uint8_t PersonalityManager::PersonalityCount() const {
  return m_personalities->PersonalityCount();
}

bool PersonalityManager::SetActivePersonality(uint8_t personality) {
  if (personality == 0 || personality > m_personalities->PersonalityCount())
    return false;
  m_active_personality = personality;
  return true;
}

const Personality *PersonalityManager::ActivePersonality() const {
  return m_personalities->Lookup(m_active_personality);
}

uint16_t PersonalityManager::ActivePersonalityFootprint() const {
  const Personality *personality = m_personalities->Lookup(
      m_active_personality);
  return personality ? personality->Footprint() : 0;
}

string PersonalityManager::ActivePersonalityDescription() const {
  const Personality *personality = m_personalities->Lookup(
      m_active_personality);
  return personality ? personality->Description() : "";
}

// Lookup a personality. Personalities are numbers from 1.
const Personality *PersonalityManager::Lookup(uint8_t personality) const {
  return m_personalities->Lookup(personality);
}

}  // namespace rdm
}  // namespace ola
#endif  // COMMON_RDM_RESPONDERPERSONALITY_H_
