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
 * ResponderOps.h
 * A framework for building RDM responders.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_resp
 * @{
 * @file ResponderOps.h
 * @brief A framework for building RDM responders.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RESPONDEROPS_H_
#define INCLUDE_OLA_RDM_RESPONDEROPS_H_

#include <ola/rdm/RDMCommand.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/RDMResponseCodes.h>

#include <map>

namespace ola {
namespace rdm {

/**
 * ResponderOps handles dispatching requests based on the PID. Have a look at
 * the dummy responder for an example.
 */
template <class Target>
class ResponderOps {
  public:
    // Each callback takes a RDMRequest object. The handler returns a
    // RDMResponse, or NULL.
    typedef const RDMResponse *(Target::*RDMHandler)(const RDMRequest *request);

    struct ParamHandler {
      uint16_t pid;
      RDMHandler get_handler;
      RDMHandler set_handler;
    };

    /*
     * param_handlers must be terminated with {0, NULL, NULL}
     */
    explicit ResponderOps(const ParamHandler param_handlers[]);

    void HandleRDMRequest(Target *target,
                          const UID &target_uid,
                          uint16_t sub_device,
                          const RDMRequest *request,
                          RDMCallback *on_complete);

  private:
    struct InternalParamHandler {
      RDMHandler get_handler;
      RDMHandler set_handler;
    };
    typedef std::map<uint16_t, InternalParamHandler> RDMHandlers;

    RDMHandlers m_handlers;

    RDMResponse *HandleSupportedParams(const RDMRequest *request);
};

}  // namespace rdm
}  // namespace ola

#include <ola/rdm/ResponderOpsPrivate.h>  // NOLINT(build/include_order)
#endif  // INCLUDE_OLA_RDM_RESPONDEROPS_H_
