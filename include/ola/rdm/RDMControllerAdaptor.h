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
 * RDMControllerAdaptor.h
 * Allows an RDMControllerInterface to be used as an
 * DiscoverableRDMControllerInterface.
 * Copyright (C) 2013 Simon Newton
 */

/**
 * @addtogroup rdm_controller
 * @{
 * @file RDMControllerAdaptor.h
 * @brief Allows an RDMControllerInterface to be used as an
 * DiscoverableRDMControllerInterface.
 * @}
 */

#ifndef INCLUDE_OLA_RDM_RDMCONTROLLERADAPTOR_H_
#define INCLUDE_OLA_RDM_RDMCONTROLLERADAPTOR_H_

#include <ola/Callback.h>
#include <ola/rdm/RDMControllerInterface.h>
#include <ola/rdm/UIDSet.h>

namespace ola {
namespace rdm {

/**
 * Adapts a RDMControllerInterface into a DiscoverableRDMControllerInterface
 */
class DiscoverableRDMControllerAdaptor
    : public DiscoverableRDMControllerInterface {
  public:
    // Ownership is not transferred.
    DiscoverableRDMControllerAdaptor(const UID &uid,
                                     RDMControllerInterface *interface)
        : m_uid(uid),
          m_interface(interface) {
    }
    void SendRDMRequest(const RDMRequest *request, RDMCallback *on_complete) {
      m_interface->SendRDMRequest(request, on_complete);
    }

    void RunFullDiscovery(RDMDiscoveryCallback *callback) {
      RunDiscovery(callback);
    }
    void RunIncrementalDiscovery(RDMDiscoveryCallback *callback) {
      RunDiscovery(callback);
    }

  private:
    const UID m_uid;
    RDMControllerInterface *m_interface;

    void RunDiscovery(RDMDiscoveryCallback *callback) {
      UIDSet uids;
      uids.AddUID(m_uid);
      callback->Run(uids);
    }
};
}  // namespace rdm
}  // namespace ola
#endif  // INCLUDE_OLA_RDM_RDMCONTROLLERADAPTOR_H_
