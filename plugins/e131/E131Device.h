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
 * E131Device.h
 * Interface for the E1.31 device
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_E131DEVICE_H
#define OLA_E131DEVICE_H

#include <olad/Device.h>
#include <olad/Plugin.h>
#include "e131/CID.h"

namespace ola {
namespace e131 {

using ola::Plugin;

class E131Device: public ola::Device {
  public:
    E131Device(Plugin *owner, const string &name,
               const ola::e131::CID &cid,
               class Preferences *preferences,
               const class PluginAdaptor *plugin_adaptor);
    ~E131Device() {}

    bool Start();
    bool Stop();
    string DeviceId() const { return "1"; }
    class E131Node *GetNode() const { return m_node; }

  private:
    class Preferences *m_preferences;
    const class PluginAdaptor *m_plugin_adaptor;
    class E131Node *m_node;
    bool m_enabled;
    ola::e131::CID m_cid;

    static const std::string IP_KEY;
};

} // e131
} // ola
#endif
