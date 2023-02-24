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
 * ResponderSlotData.h
 * Manages slot data for a personality for a RDM responder.
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderSlotData.h
 * @brief Holds the information about DMX slots.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RESPONDERSLOTDATA_H_
#define INCLUDE_OLA_RDM_RESPONDERSLOTDATA_H_

#include <ola/rdm/RDMEnums.h>

#include <stdint.h>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * @brief Holds information about a single DMX slot.
 */
class SlotData {
 public:
    /**
     * @brief The Slot Type.
     * Used in the SLOT_INFO message.
     * @returns the slot type.
     */
    rdm_slot_type SlotType() const { return m_slot_type; }

    /**
     * @brief The Slot ID Definition.
     * Used in the SLOT_INFO message. This can either be a rdm_slot_definition
     * for a primary slot, or the index of the primary slot in the case of a
     * secondary slot.
     * @returns The slot ID Definition.
     */
    uint16_t SlotIDDefinition() const { return m_slot_id; }

    /**
     * @brief The default slot value.
     * Used in the DEFAULT_SLOT_VALUE message.
     * @returns the default slot value.
     */
    uint8_t DefaultSlotValue() const { return m_default_slot_value; }

    /**
     * @brief true if there is a description for this slot, false otherwise.
     * @returns true if there is a description for this slot.
     */
    bool HasDescription() const { return m_has_description; }

    /**
     * @brief The slot description.
     * Used in the SLOT_DESCRIPTION message.
     * @returns the slot description.
     */
    std::string Description() const { return m_description; }

    /**
     * @brief Create a new Primary slot
     * @param slot_definition the slot id definition.
     * @param default_slot_value the default value for the slot
     * @returns a SlotData object.
     */
    static SlotData PrimarySlot(
        rdm_slot_definition slot_definition,
        uint8_t default_slot_value);

    /**
     * @brief Create a new Primary slot with a description
     * @param slot_definition the slot id definition.
     * @param default_slot_value the default value for the slot
     * @param description the slot description
     * @returns a SlotData object.
     */
    static SlotData PrimarySlot(
        rdm_slot_definition slot_definition,
        uint8_t default_slot_value,
        const std::string &description);

    /**
     * @brief Create a new Secondary slot.
     * @param slot_type the secondary slot type
     * @param primary_slot the primary slot index.
     * @param default_slot_value the default value for the slot
     * @returns a SlotData object.
     */
    static SlotData SecondarySlot(
        rdm_slot_type slot_type,
        uint16_t primary_slot,
        uint8_t default_slot_value);

    /**
     * @brief Create a new Secondary slot with a description.
     * @param slot_type the secondary slot type
     * @param primary_slot the primary slot index.
     * @param default_slot_value the default value for the slot
     * @param description the slot description
     * @returns a SlotData object.
     */
    static SlotData SecondarySlot(
        rdm_slot_type slot_type,
        uint16_t primary_slot,
        uint8_t default_slot_value,
        const std::string &description);

 private:
    SlotData(rdm_slot_type slot_type,
             uint16_t slot_id,
             uint8_t default_slot_value);

    SlotData(rdm_slot_type slot_type,
             uint16_t slot_id,
             uint8_t default_slot_value,
             const std::string &description);

    rdm_slot_type m_slot_type;
    uint16_t m_slot_id;
    uint8_t m_default_slot_value;
    bool m_has_description;
    std::string m_description;
};


/**
 * @brief Holds information about a set of slots.
 */
class SlotDataCollection {
 public:
    typedef std::vector<SlotData> SlotDataList;

    explicit SlotDataCollection(const SlotDataList &slot_data);
    SlotDataCollection() {}

    /**
     * @brief The number of slots we have information for.
     * @returns the number of slots we have information for.
     */
    uint16_t SlotCount() const;

    /**
     * @brief Lookup slot data based on the slot index
     * @returns A pointer to a SlotData object, or null if no such slot exists.
     */
    const SlotData *Lookup(uint16_t slot) const;

 private:
    SlotDataList m_slot_data;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RESPONDERSLOTDATA_H_
