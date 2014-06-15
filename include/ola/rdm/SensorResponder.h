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
 * SensorResponder_h
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file SensorResponder.h
 * @brief A soft RDM responder that has sensors
 * @}
 */

#ifndef INCLUDE_OLA_RDM_SENSORRESPONDER_H_
#define INCLUDE_OLA_RDM_SENSORRESPONDER_H_

#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMEnums.h>
#include <ola/rdm/ResponderOps.h>
#include <ola/rdm/ResponderSensor.h>
#include <ola/rdm/UID.h>

#include <string>
#include <vector>

namespace ola {
namespace rdm {

/**
 * A simulated responder with no footprint and just sensors.
 */
class SensorResponder: public RDMControllerInterface {
 public:
  explicit SensorResponder(const UID &uid);
  ~SensorResponder();

  void SendRDMRequest(const RDMRequest *request, RDMCallback *callback);

 private:
  /**
   * The RDM Operations for the SensorResponder.
   */
  class RDMOps : public ResponderOps<SensorResponder> {
   public:
    static RDMOps *Instance() {
      if (!instance)
        instance = new RDMOps();
      return instance;
    }

   private:
    RDMOps() : ResponderOps<SensorResponder>(PARAM_HANDLERS) {}

    static RDMOps *instance;
  };

  const UID m_uid;
  bool m_identify_mode;
  Sensors m_sensors;

  const RDMResponse *GetDeviceInfo(const RDMRequest *request);
  const RDMResponse *GetProductDetailList(const RDMRequest *request);
  const RDMResponse *GetIdentify(const RDMRequest *request);
  const RDMResponse *SetIdentify(const RDMRequest *request);
  const RDMResponse *GetManufacturerLabel(const RDMRequest *request);
  const RDMResponse *GetDeviceLabel(const RDMRequest *request);
  const RDMResponse *GetDeviceModelDescription(const RDMRequest *request);
  const RDMResponse *GetSoftwareVersionLabel(const RDMRequest *request);
  const RDMResponse *GetSensorDefinition(const RDMRequest *request);
  const RDMResponse *GetSensorValue(const RDMRequest *request);
  const RDMResponse *SetSensorValue(const RDMRequest *request);
  const RDMResponse *RecordSensor(const RDMRequest *request);

  static const ResponderOps<SensorResponder>::ParamHandler PARAM_HANDLERS[];
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_SENSORRESPONDER_H_
