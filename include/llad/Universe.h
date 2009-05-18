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
 * Universe.h
 * Header file for the Universe class, see Universe.cpp for details.
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <set>
#include <vector>
#include <string>
#include <lla/ExportMap.h>
#include <lla/DmxBuffer.h>

namespace lla {

using namespace std;
class AbstractPort;

class Universe {
  public:
    enum merge_mode {
      MERGE_HTP,
      MERGE_LTP
    };

    Universe(unsigned int uid, class UniverseStore *store,
             ExportMap *export_map);
    ~Universe();

    // Properties for this universe
    string Name() const { return m_universe_name; }
    unsigned int UniverseId() const { return m_universe_id; }
    merge_mode MergeMode() const { return m_merge_mode; }
    bool IsActive() const;

    // Used to adjust the properties
    void SetName(const string &name);
    void SetMergeMode(merge_mode merge_mode);

    // Each universe has a DMXBuffer
    bool SetDMX(const DmxBuffer &buffer);
    const DmxBuffer &GetDMX() const { return m_buffer; }

    // These are the ports we need to nofity when data changes
    bool AddPort(class AbstractPort *port);
    bool RemovePort(class AbstractPort *port);
    bool ContainsPort(class AbstractPort *port) const;
    int PortCount() const { return m_ports.size(); }

    // Source clients are those that provide us with data
    bool AddSourceClient(class Client *client);
    bool RemoveSourceClient(class Client *client);
    bool ContainsSourceClient(class Client *client) const;
    unsigned int SourceClientCount() const { return m_source_clients.size(); }

    // Sink clients are those that we need to send data
    bool AddSinkClient(class Client *client);
    bool RemoveSinkClient(class Client *client);
    bool ContainsSinkClient(class Client *client) const;
    unsigned int SinkClientCount() const { return m_sink_clients.size(); }

    // These are called when new data arrives on a port/client
    bool PortDataChanged(AbstractPort *port);
    bool SourceClientDataChanged(class Client *client);

    bool operator==(const Universe &other) {
      return m_universe_id == other.UniverseId();
    }

    static const string K_UNIVERSE_NAME_VAR;
    static const string K_UNIVERSE_MODE_VAR;
    static const string K_UNIVERSE_PORT_VAR;
    static const string K_UNIVERSE_SOURCE_CLIENTS_VAR;
    static const string K_UNIVERSE_SINK_CLIENTS_VAR;
    static const string K_MERGE_HTP_STR;
    static const string K_MERGE_LTP_STR;

  private:
    Universe(const Universe&);
    Universe& operator=(const Universe&);
    bool UpdateDependants();
    void UpdateName();
    void UpdateMode();
    bool RemoveClient(class Client *client, bool is_source);
    bool AddClient(class Client *client, bool is_source);
    bool HTPMergeAllSources();

    string m_universe_name;
    unsigned int m_universe_id;
    string m_universe_id_str;
    enum merge_mode m_merge_mode; // merge mode
    vector<class AbstractPort*> m_ports; // ports patched to this universe
    set<class Client*> m_sink_clients; // clients that require updates
    set<class Client*> m_source_clients; // clients that provide data
    class UniverseStore *m_universe_store;

    DmxBuffer m_buffer;
    ExportMap *m_export_map;
};

} //lla
#endif
