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
 * ACNVectors.h
 * Vectors used in ACN PDUs
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PLUGINS_E131_E131_ACNVECTORS_H_
#define PLUGINS_E131_E131_ACNVECTORS_H_

#include <stdint.h>

namespace ola {
namespace plugin {
namespace e131 {

// Root vectors
enum RootVector {
  VECTOR_ROOT_E131_REV2 = 3,  // used by some very old gear
  VECTOR_ROOT_E131 = 4,
  VECTOR_ROOT_E133 = 5,
  VECTOR_ROOT_NULL = 6,
};

// DMP Vectors
enum DMPVector {
  DMP_GET_PROPERTY_VECTOR = 1,
  DMP_SET_PROPERTY_VECTOR = 2,
};

// E1.31 vectors
enum E131Vector {
  VECTOR_E131_DMP = 2,
};

// E1.33 vectors
enum E133Vector {
  VECTOR_FRAMING_RDMNET = 1,
  VECTOR_FRAMING_STATUS = 2,
  VECTOR_FRAMING_CONTROLLER = 3,
  VECTOR_FRAMING_CHANGE_NOTIFICATION = 4,
};

// E1.33 Controller Vectors
enum E133ControllerVector {
  VECTOR_CONTROLLER_FETCH_DEVICES = 1,
  VECTOR_CONTROLLER_DEVICE_LIST = 2,
  VECTOR_CONTROLLER_DEVICE_ACQUIRED = 3,
  VECTOR_CONTROLLER_DEVICE_RELEASED = 4,
  VECTOR_CONTROLLER_EXPECT_MASTER = 5,
};
}  // e131
}  // plugin
}  // ola
#endif  // PLUGINS_E131_E131_ACNVECTORS_H_
