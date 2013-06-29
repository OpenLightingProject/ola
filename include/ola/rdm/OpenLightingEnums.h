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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * OpenLightingEnums.h
 * Provide OLA's RDM Manufacturer PIDs & Model IDs.
 * Copyright (C) 2013 Peter Newman
 */

#ifndef INCLUDE_OLA_RDM_OPENLIGHTINGENUMS_H_
#define INCLUDE_OLA_RDM_OPENLIGHTINGENUMS_H_

#include <stdint.h>

namespace ola {
namespace rdm {

/**
 * Please discuss on open-lighting@googlegroups.com before claiming additional
 * manufacturer PIDs and update http://opendmx.net/index.php/Open_Lighting_PIDs
 * ANSI E1.20 section 6.2.10.2 Parameter ID (PID) for info on assigning
 * manufacturer PIDs, although we're not currently sticking entirely to the
 * specification, like a number of other companies
 */
typedef enum {
  // OLA Arduino
  OLA_MANUFACTURER_PID_SERIAL_NUMBER = 0x8000,
  // OLA Dummy RDM Responder
  OLA_MANUFACTURER_PID_CODE_VERSION = 0x8001,
} rdm_ola_manufacturer_pid;

typedef enum {
  // OLA Dummy RDM Responder
  OLA_DUMMY_DEVICE_MODEL = 1,
  // Arduino RGB Mixer
  //OLA_RGB_MIXER_MODEL = 2,
  // SPI Device
  OLA_SPI_DEVICE_MODEL = 3,
  // Dummy Dimmer
  OLA_DUMMY_DIMMER_MODEL = 4,
  // Dummy Moving Light
  OLA_DUMMY_MOVING_LIGHT_MODEL = 5,
} ola_rdm_model_id;

}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_OPENLIGHTINGENUMS_H_
