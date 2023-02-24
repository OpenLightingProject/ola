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
 * DimmerResponder.h
 * Simulates a RDM enabled dimmer with sub devices.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file DimmerResponder.h
 * @brief A soft responder that acts like an RDM enabled dimmer with sub
 * devices.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_DIMMERRESPONDER_H_
#define INCLUDE_OLA_RDM_DIMMERRESPONDER_H_

#include <ola/rdm/DimmerRootDevice.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/SubDeviceDispatcher.h>
#include <ola/rdm/UID.h>

#include <map>
#include <memory>

namespace ola {
namespace rdm {

/**
 * A RDM responder that simulates a dimmer rack. This has a configurable number
 * of sub-devices.
 */
class DimmerResponder: public RDMControllerInterface {
 public:
  DimmerResponder(const UID &uid, uint16_t number_of_subdevices);
  virtual ~DimmerResponder();

  void SendRDMRequest(RDMRequest *request, RDMCallback *callback);

 private:
  SubDeviceDispatcher m_dispatcher;
  std::auto_ptr<DimmerRootDevice> m_root_device;
  std::map<uint16_t, class DimmerSubDevice*> m_sub_devices;
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_DIMMERRESPONDER_H_
