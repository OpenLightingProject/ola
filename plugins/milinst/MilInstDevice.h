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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * MilInstDevice.h
 * Interface for the milinst device
 * Copyright (C) 2013 Peter Newman
 */

#ifndef PLUGINS_MILINST_MILINSTDEVICE_H_
#define PLUGINS_MILINST_MILINSTDEVICE_H_

#include <memory>
#include <string>
#include "ola/DmxBuffer.h"
#include "ola/network/Socket.h"
#include "olad/Device.h"
#include "plugins/milinst/MilInstWidget.h"

namespace ola {

class AbstractPlugin;

namespace plugin {
namespace milinst {

using ola::Device;
using std::auto_ptr;

class MilInstDevice: public Device {
  public:
    MilInstDevice(AbstractPlugin *owner,
                  const string &name,
                  const string &dev_path);

    string DeviceId() const { return m_path; }
    ola::io::ConnectedDescriptor *GetSocket() const;

  protected:
    bool StartHook();
    void PrePortStop();

  private:
    string m_path;
    auto_ptr<MilInstWidget> m_widget;
};
}  // namespace milinst
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_MILINST_MILINSTDEVICE_H_
