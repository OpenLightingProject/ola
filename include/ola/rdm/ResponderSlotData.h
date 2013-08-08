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
 * ResponderSlotData.h
 * Manages slot data for a personality for a RDM responder.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERSLOTDATA_H_
#define INCLUDE_OLA_RDM_RESPONDERSLOTDATA_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "ola/rdm/RDMEnums.h"

namespace ola {
namespace rdm {

using std::string;

/**
 * Represents slot data.
 */
class SlotData {
  public:
    SlotData(rdm_slot_type slot_type,
             uint8_t default_slot_value,
             const string &description = "");

    rdm_slot_type SlotType() const { return m_slot_type; }
    virtual uint16_t RawSlotDefinition() const = 0;
    uint8_t DefaultSlotValue() const { return m_default_slot_value; }
    string Description() const { return m_description; }
    bool IsPrimary() const { return (m_slot_type == ST_PRIMARY); }

  private:
    rdm_slot_type m_slot_type;
    uint8_t m_default_slot_value;
    string m_description;
};


class PrimarySlotData: public SlotData {
  public:
    PrimarySlotData(rdm_slot_definition slot_definition,
                    uint8_t default_slot_value,
                    const string &description = "");

    uint16_t RawSlotDefinition() const {
      return static_cast<uint16_t>(m_slot_definition);
    }
    rdm_slot_definition SlotDefinition() const { return m_slot_definition; }

  private:
    rdm_slot_definition m_slot_definition;
};


class SecondarySlotData: public SlotData {
  public:
    SecondarySlotData(rdm_slot_type slot_type,
                      uint16_t slot_definition,
                      uint8_t default_slot_value,
                      const string &description = "");

    uint16_t RawSlotDefinition() const { return m_slot_definition; }

  private:
    uint16_t m_slot_definition;
};


/**
 * Holds the list of slot data for a personality for a class of responder. A
 * single instance is shared between all responders of the same type.
 */
class SlotDataCollection {
  public:
    typedef std::vector<SlotData*> SlotDataList;

    explicit SlotDataCollection(const SlotDataList &slot_data);
    SlotDataCollection() {}  // Create an empty slot data collection

    uint16_t SlotDataCount() const;

    SlotData *Lookup(uint16_t slot) const;

  private:
    SlotDataList m_slot_data;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERSLOTDATA_H_
