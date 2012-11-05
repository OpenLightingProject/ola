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
#include <ola/ExportMap.h>  // NOLINT
#include <ola/io/SelectServerInterface.h>  // NOLINT

namespace ola {

using ola::thread::timeout_id;

class PluginAdaptor: public ola::io::SelectServerInterface {
  public:
    PluginAdaptor(class DeviceManager *device_manager,
                  ola::io::SelectServerInterface *select_server,
                  ExportMap *export_map,
                  class PreferencesFactory *preferences_factory,
                  class PortBrokerInterface *port_broker);

    // The following methods are part of the SelectServerInterface
    bool AddReadDescriptor(ola::io::ReadFileDescriptor *descriptor);
    bool AddReadDescriptor(ola::io::ConnectedDescriptor *descriptor,
                   bool delete_on_close = false);
    bool RemoveReadDescriptor(ola::io::ReadFileDescriptor *descriptor);
    bool RemoveReadDescriptor(ola::io::ConnectedDescriptor *descriptor);
    bool AddWriteDescriptor(ola::io::WriteFileDescriptor *descriptor);
    bool RemoveWriteDescriptor(ola::io::WriteFileDescriptor *descriptor);

    timeout_id RegisterRepeatingTimeout(unsigned int ms,
                                        Callback0<bool> *closure);
    timeout_id RegisterRepeatingTimeout(const TimeInterval &interval,
                                        Callback0<bool> *closure);
    timeout_id RegisterSingleTimeout(unsigned int ms,
                                     SingleUseCallback0<void> *closure);
    timeout_id RegisterSingleTimeout(const TimeInterval &interval,
                                     SingleUseCallback0<void> *closure);
    void RemoveTimeout(timeout_id id);

    void Execute(ola::BaseCallback0<void> *closure);

    const TimeStamp *WakeUpTime() const;

    ExportMap *GetExportMap() const {
      return m_export_map;
    }

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
    ola::io::SelectServerInterface *m_ss;
    ExportMap *m_export_map;
    class PreferencesFactory *m_preferences_factory;
    class PortBrokerInterface *m_port_broker;
};
}  // ola
#endif  // INCLUDE_OLAD_PLUGINADAPTOR_H_
