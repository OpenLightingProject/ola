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
 * @brief A class which dispatches RDM requests to registered PID handlers.
 *
 * ResponderOps is a stateless RDM request dispatcher. The constructor takes a
 * list of parameter handlers in the form of pointers to member functions. When
 * HandleRDMRequest is called, it invokes the registered handler after
 * performing a common set of checks. If no handler is found, a response
 * containing NR_UNKNOWN_PID is returned.
 *
 * The stateless nature of ResponderOps means a single ResponderOps
 * object can handle requests for all responders of the same type. This
 * conserves memory when large numbers of responders are active.
 *
 * ResponderOps handles SUPPORTED_PARAMETERS internally, however this can be
 * overridden by registering a handler for SUPPORTED_PARAMETERS.
 *
 * @tparam Target the object to invoke the PID handlers on.
 */
template <class Target>
class ResponderOps {
 public:
    /**
     * @brief The member function to call on the target to handle a request.
     *
     * The member function should return a RDMResponse object. If the request
     * was broadcast, this object will be discarded.
     */
    typedef const RDMResponse *(Target::*RDMHandler)(const RDMRequest *request);

    /**
     * @brief the structure that defines the behaviour for a specific PID.o
     *
     * Either the get_handler or set_handlers may be NULL if the command
     * class isn't defined for this PID.
     */
    struct ParamHandler {
      uint16_t pid;  /**< The PID this handler is for */
      RDMHandler get_handler;  /**< The method used to handle GETs */
      RDMHandler set_handler;  /**< The method used to handle SETs */
    };

    /**
     * @brief Construct a new ResponderOps object.
     * @param param_handlers an array of ParamHandlers. Must be terminated with
     * {0, NULL, NULL}
     * @param include_required_pids if true, the internal SUPPORTED_PARAMETERS
     *   handler includes those PIDs that are marked a required in E1.20. This
     *   is required for sub-devices, see Section 2 of E1.37.
     */
    explicit ResponderOps(const ParamHandler param_handlers[],
                          bool include_required_pids = false);

    /**
     * @brief Handle a RDMRequest
     * @param target the target object to invoke the registered handler on
     * @param target_uid the UID of the target
     * @param sub_device the sub_device of the target
     * @param request the RDM request object
     * @param on_complete the callback to run when the request completes.
     */
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

    bool m_include_required_pids;
    RDMHandlers m_handlers;

    RDMResponse *HandleSupportedParams(const RDMRequest *request);
};

}  // namespace rdm
}  // namespace ola
#include <ola/rdm/ResponderOpsPrivate.h>  // NOLINT(build/include_order)
#endif  // INCLUDE_OLA_RDM_RESPONDEROPS_H_
