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
 * PluginAdaptor.h
 * The provides operations on a ola_device.
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef INCLUDE_OLAD_PLUGINADAPTOR_H_
#define INCLUDE_OLAD_PLUGINADAPTOR_H_

#include <string>
#include <ola/Clock.h>  // NOLINT
#include <ola/Callback.h>  // NOLINT
#include <ola/network/SelectServerInterface.h>  // NOLINT

namespace ola {

using ola::thread::timeout_id;

class PluginAdaptor: public ola::network::SelectServerInterface {
  public:
    PluginAdaptor(class DeviceManager *device_manager,
                  ola::network::SelectServerInterface *select_server,
                  class PreferencesFactory *preferences_factory,
                  class PortBrokerInterface *port_broker);

    // The following methods are part of the SelectServerInterface
    bool AddReadDescriptor(ola::network::ReadFileDescriptor *descriptor);
    bool AddReadDescriptor(ola::network::ConnectedDescriptor *descriptor,
                   bool delete_on_close = false);
    bool RemoveReadDescriptor(ola::network::ReadFileDescriptor *descriptor);
    bool RemoveReadDescriptor(ola::network::ConnectedDescriptor *descriptor);
    bool AddWriteDescriptor(ola::network::WriteFileDescriptor *descriptor);
    bool RemoveWriteDescriptor(ola::network::WriteFileDescriptor *descriptor);

    timeout_id RegisterRepeatingTimeout(unsigned int ms,
                                        Callback0<bool> *closure);
    timeout_id RegisterSingleTimeout(unsigned int ms,
                                     SingleUseCallback0<void> *closure);
    void RemoveTimeout(timeout_id id);

    void Execute(ola::BaseCallback0<void> *closure);

    const TimeStamp *WakeUpTime() const;

    // These are the extra bits for the plugins
    bool RegisterDevice(class AbstractDevice *device) const;
    bool UnregisterDevice(class AbstractDevice *device) const;
    class Preferences *NewPreference(const std::string &name) const;
    class PortBrokerInterface *GetPortBroker() const {
      return m_port_broker;
    }

  private:
    PluginAdaptor(const PluginAdaptor&);
    PluginAdaptor& operator=(const PluginAdaptor&);

    DeviceManager *m_device_manager;
    ola::network::SelectServerInterface *m_ss;
    class PreferencesFactory *m_preferences_factory;
    class PortBrokerInterface *m_port_broker;
};
}  // ola
#endif  // INCLUDE_OLAD_PLUGINADAPTOR_H_
