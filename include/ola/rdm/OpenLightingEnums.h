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
 * OpenLightingEnums.h
 * Provide OLA's RDM Manufacturer PIDs & Model IDs.
 * Copyright (C) 2013 Peter Newman
 */

/**
 * @addtogroup rdm_helpers
 * @{
 * @file OpenLightingEnums.h
 * @brief Provide OLA's RDM Manufacturer PIDs & Model IDs.
 * @}
 */
#ifndef INCLUDE_OLA_RDM_OPENLIGHTINGENUMS_H_
#define INCLUDE_OLA_RDM_OPENLIGHTINGENUMS_H_

#include <stdint.h>

namespace ola {
namespace rdm {

/**
 * Please discuss on open-lighting@googlegroups.com before claiming additional
 * manufacturer PIDs and update
 * https://wiki.openlighting.org/index.php/Open_Lighting_PIDs
 * N.B. This list may only include PIDs relevant to OLA, not other Open
 * Lighting Project products.
 * See ANSI E1.20 section 6.2.10.2 Parameter ID (PID) for info on assigning
 * manufacturer PIDs, although we're not currently sticking entirely to the
 * specification, like a number of other companies
 */
typedef enum {
  // OLA Arduino
  OLA_MANUFACTURER_PID_SERIAL_NUMBER = 0x8000,
  // OLA Dummy RDM Responder
  OLA_MANUFACTURER_PID_CODE_VERSION = 0x8001,
} rdm_ola_manufacturer_pid;

/**
 * Also see the list here
 * https://wiki.openlighting.org/index.php/Open_Lighting_Allocations#RDM_Model_Numbers
 */
typedef enum {
  // OLA Dummy RDM Responder
  OLA_DUMMY_DEVICE_MODEL = 1,
  // Arduino RGB Mixer
  // OLA_RGB_MIXER_MODEL = 2,
  // SPI Device
  OLA_SPI_DEVICE_MODEL = 3,
  // Dummy Dimmer
  OLA_DUMMY_DIMMER_MODEL = 4,
  // Dummy Moving Light
  OLA_DUMMY_MOVING_LIGHT_MODEL = 5,
  // A responder which ack timers
  OLA_ACK_TIMER_MODEL = 6,
  // A sensor only responder
  OLA_SENSOR_ONLY_MODEL = 7,
  // A E1.37 Dimmer
  OLA_E137_DIMMER_MODEL = 8,
  // A E1.37-2 responder
  OLA_E137_2_MODEL = 9,
} ola_rdm_model_id;

extern const char OLA_MANUFACTURER_LABEL[];
extern const char OLA_MANUFACTURER_URL[];
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_OPENLIGHTINGENUMS_H_
