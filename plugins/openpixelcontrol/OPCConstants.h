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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * OPCConstants.h
 * Constants for the Open Pixel Control Protocol.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef PLUGINS_OPENPIXELCONTROL_OPCCONSTANTS_H_
#define PLUGINS_OPENPIXELCONTROL_OPCCONSTANTS_H_

#include <stdint.h>

#include "ola/Constants.h"

namespace ola {
namespace plugin {
namespace openpixelcontrol {

enum {
  /**
   * @brief The size of the OPC frame header.
   */
  OPC_HEADER_SIZE = 4,

  /**
   * @brief The size of an OPC frame with DMX512 data.
   */
  OPC_FRAME_SIZE = DMX_UNIVERSE_SIZE + OPC_HEADER_SIZE,
};

/**
 * @brief The set-pixel command
 */
static const uint8_t SET_PIXEL_COMMAND = 0;

}  // namespace openpixelcontrol
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_OPENPIXELCONTROL_OPCCONSTANTS_H_
