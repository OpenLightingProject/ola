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
 * FtdiDmxPort.h
 * The FTDI usb chipset DMX plugin for ola
 * Copyright (C) 2011 Rui Barreiros
 *
 * Additional modifications to enable support for multiple outputs and
 * additional device ids did change the original structure.
 *
 * by E.S. Rosenberg a.k.a. Keeper of the Keys 5774/2014
 */

#ifndef PLUGINS_FTDIDMX_FTDIDMXPORT_H_
#define PLUGINS_FTDIDMX_FTDIDMXPORT_H_

#include <string>

#include "ola/DmxBuffer.h"
#include "ola/rdm/DiscoveryAgent.h"
#include "ola/rdm/RDMResponseCodes.h"

#include "olad/Port.h"
#include "olad/Preferences.h"

#include "plugins/ftdidmx/FtdiDmxDevice.h"
#include "plugins/ftdidmx/FtdiWidget.h"
#include "plugins/ftdidmx/FtdiDmxThread.h"

namespace ola {
namespace plugin {
namespace ftdidmx {

class FtdiDmxOutputPort
    : public ola::BasicOutputPort,
      public ola::rdm::DiscoveryTargetInterface {
 public:
    FtdiDmxOutputPort(FtdiDmxDevice *parent,
                      FtdiInterface *interface,
                      unsigned int id,
                      unsigned int freq)
      : BasicOutputPort(parent, id, false, true),
          m_interface(interface),
          m_thread(interface, freq),
          m_transaction_number(0),
          m_discovery_agent(this),
          m_uid(0x7a7012345678),
          m_mute_complete(nullptr),
          m_unmute_complete(nullptr),
          m_branch_callback(nullptr) {
      m_thread.Start();
    }
    ~FtdiDmxOutputPort() {
      m_thread.Stop();
      delete m_interface;
    }

    bool WriteDMX(const ola::DmxBuffer &buffer, uint8_t) {
      return m_thread.WriteDMX(buffer);
    }

    void SendRDMRequest(ola::rdm::RDMRequest *request,
                        ola::rdm::RDMCallback *callback) {
      request->SetTransactionNumber(m_transaction_number++);
      m_thread.SendRDMRequest(request, callback);
    }

    std::string Description() const { return m_interface->Description(); }

    void on_mute_reply(RDMReply *mute_reply) {
      MuteDeviceCallback *my_mute_complete = m_mute_complete;
      m_mute_complete = nullptr;
      //if(mute_reply->Response().SourceUID() == )
      my_mute_complete->Run(true);
    }

    void on_unmute_complete(RDMReply *unmute_reply) {
      UnMuteDeviceCallback *my_unmute_complete = m_unmute_complete;
      m_unmute_complete = nullptr;
      ola::rdm::rdm_response_code response = unmute_reply->StatusCode();
      if(response == rdm::RDM_WAS_BROADCAST || response == rdm::RDM_COMPLETED_OK) {
        my_unmute_complete->Run();
      } else {
        OLA_WARN << "Something went wrong broadcasting unmute";
        my_unmute_complete->Run();
      }
    }

    void on_branch_callback(RDMReply *branch_reply) {
      BranchCallback *my_branch_callback = m_branch_callback;
      m_branch_callback = nullptr;
      my_branch_callback->Run(branch_reply->frame().data, branch_reply->frame().length);
    }

    void MuteDevice(const ola::rdm::UID &target,
                    MuteDeviceCallback *mute_complete){
      if(m_mute_complete == nullptr) {
        m_mute_complete = mute_complete;
        m_thread.SendRDMRequest(ola::rdm::NewMuteRequest(m_uid, target, m_transaction_number++),
                                &on_mute_reply);
      } else {
        //fail
      }
    }
    void UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
      if(m_unmute_complete == nullptr) {
        m_unmute_complete = unmute_complete;
        m_thread.SendRDMRequest(ola::rdm::NewUnMuteRequest(m_uid, ola::rdm::UID::AllDevices(),
                                                           m_transaction_number++),
                                [&FtdiDmxOutputPort](RDMRequest*) { return on_unmute_complete(RDMReply *unmute_reply)) };
      } else {
        //fail
      }
    }
    void Branch(const ola::rdm::UID &lower,
                const ola::rdm::UID &upper,
                BranchCallback *callback) {
      if(m_branch_callback == nullptr) {
        m_branch_callback = callback;
        m_thread.SendRDMRequest(ola::rdm::NewDiscoveryUniqueBranchRequest(m_uid, lower, upper,
                                                                          m_transaction_number++),
                                &on_branch_callback);
      } else {
        //fail
      }
    }

  private:
    FtdiInterface *m_interface;
    FtdiDmxThread m_thread;

    uint8_t m_transaction_number;
    ola::rdm::DiscoveryAgent m_discovery_agent;
    const ola::rdm::UID m_uid;

    MuteDeviceCallback *m_mute_complete;
    UnMuteDeviceCallback * m_unmute_complete;
    BranchCallback * m_branch_callback;

};
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIDMXPORT_H_
