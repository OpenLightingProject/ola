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
 * ACNVectors.h
 * Vectors used in ACN PDUs
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_ACN_ACNVECTORS_H_
#define INCLUDE_OLA_ACN_ACNVECTORS_H_

/**
 * @addtogroup acn
 * @{
 * @file ACNVectors.h
 * @brief ACN Vector values.
 * @}
 */

#include <stdint.h>

namespace ola {
namespace acn {

/**
 * @addtogroup acn
 * @{
 */

/**
 * @brief ACN vectors used at the root layer.
 */
enum RootVector {
  VECTOR_ROOT_E131_REV2 = 3,  /**< Draft E1.31, used by some old gear. */
  VECTOR_ROOT_E131 = 4,  /**< E1.31 (sACN) */
  VECTOR_ROOT_E133 = 5,  /**< E1.33 (RDNNet) */
  VECTOR_ROOT_NULL = 6,  /**< NULL (empty) root */
};

/**
 * @brief Vectors used at the DMP layer.
 */
enum DMPVector {
  DMP_GET_PROPERTY_VECTOR = 1,  /**< DMP Get */
  DMP_SET_PROPERTY_VECTOR = 2,  /**< DMP Set */
};

/**
 * @brief Vectors used at the E1.31 layer.
 */
enum E131Vector {
  VECTOR_E131_DATA = 2,  /**< DMP data (DATA_PACKET_VECTOR) */
  VECTOR_E131_SYNC = 3,  /**< Sync data (SYNC_PACKET_VECTOR) */
  VECTOR_E131_DISCOVERY = 4,  /**< Discovery data (DISCOVERY_PACKET_VECTOR) */
};

/**
 * @brief Vectors used at the E1.33 layer.
 */
enum E133Vector {
  VECTOR_FRAMING_RDMNET = 1,  /**< RDMNet data */
  VECTOR_FRAMING_STATUS = 2,  /**< Status message */
  VECTOR_FRAMING_CONTROLLER = 3,  /**< Controller message */
  VECTOR_FRAMING_CHANGE_NOTIFICATION = 4,  /**< Controller change message */
};

/**
 * @brief Vectors used at the E1.33 Controller layer.
 */
enum E133ControllerVector {
  VECTOR_CONTROLLER_FETCH_DEVICES = 1,  /**< Fetch devices message */
  VECTOR_CONTROLLER_DEVICE_LIST = 2,  /**< Device list message */
  VECTOR_CONTROLLER_DEVICE_ACQUIRED = 3,  /**< Device acquired message */
  VECTOR_CONTROLLER_DEVICE_RELEASED = 4,  /**< Device released message */
  VECTOR_CONTROLLER_EXPECT_MASTER = 5,  /**< Expect master message */
};

/**
 * @}
 */
}  // namespace acn
}  // namespace ola

#endif  // INCLUDE_OLA_ACN_ACNVECTORS_H_
