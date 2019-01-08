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
          m_thread(interface, freq) {
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
      m_thread.SendRDMRequest(request, callback);
    }

    void RunFullDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_thread.RunFullDiscovery(callback);
    }
    void RunIncrementalDiscovery(ola::rdm::RDMDiscoveryCallback *callback) {
      m_thread.RunIncrementalDiscovery(callback);
    }

    std::string Description() const { return m_interface->Description(); }

    void MuteDevice(const ola::rdm::UID &target,
                    MuteDeviceCallback *mute_complete){
      OLA_WARN << "Port.MuteDevice()";
      m_thread.MuteDevice(target, mute_complete);
    }

    void UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
      OLA_WARN << "Port.UnMuteAll()";
      m_thread.UnMuteAll(unmute_complete);
    }

    void Branch(const ola::rdm::UID &lower,
                const ola::rdm::UID &upper,
                BranchCallback *callback) {
      OLA_WARN << "Port.Branch()";
      m_thread.Branch(lower, upper, callback);
    }

  private:
    FtdiInterface *m_interface;
    FtdiDmxThread m_thread;

};
}  // namespace ftdidmx
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_FTDIDMX_FTDIDMXPORT_H_
