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
 * StageProfiDevice.h
 * Interface for the stageprofi device
 * Copyright (C) 2006-2009 Simon Newton
 */

#ifndef PLUGINS_STAGEPROFI_STAGEPROFIDEVICE_H_
#define PLUGINS_STAGEPROFI_STAGEPROFIDEVICE_H_

#include <string>
#include "ola/DmxBuffer.h"
#include "ola/network/Socket.h"
#include "olad/Device.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace stageprofi {

using ola::Device;
using ola::network::Socket;

class StageProfiDevice: public Device {
  public:
    StageProfiDevice(AbstractPlugin *owner,
                     const string &name,
                     const string &dev_path);
    ~StageProfiDevice();

    bool Start();
    bool Stop();
    bool AllowLooping() const { return false; }
    bool AllowMultiPortPatching() const { return false; }
    // I don't think this get us full stickiness because USB devices may
    // appear as different devices.
    string DeviceId() const { return m_path; }
    ola::network::ConnectedSocket *GetSocket() const;

  private:
    string m_path;
    bool m_enabled;
    class StageProfiWidget *m_widget;
};
}  // stageprofi
}  // plugin
}  // ola
#endif  // PLUGINS_STAGEPROFI_STAGEPROFIDEVICE_H_
