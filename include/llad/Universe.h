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
 * Header file for the Universe class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef UNIVERSE_H
#define UNIVERSE_H

#include <stdint.h>
#include <vector>
#include <string>
#include <lla/BaseTypes.h>
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

    string Name() const { return m_universe_name; }
    void SetName(const string &name);
    merge_mode MergeMode() const { return m_merge_mode; }
    void SetMergeMode(merge_mode merge_mode);
    unsigned int UniverseId() const { return m_universe_id; }
    bool IsActive() const;

    bool AddPort(class AbstractPort *prt);
    bool RemovePort(class AbstractPort *prt);
    int PortCount() const { return m_ports.size(); }

    bool AddClient(class Client *client);
    bool RemoveClient(class Client *client);
    bool ContainsClient(class Client *client) const;
    unsigned int ClientCount() const { return m_clients.size(); }

    bool SetDMX(const DmxBuffer &buffer);
    const DmxBuffer &GetDMX() const { return m_buffer; }
    bool PortDataChanged(AbstractPort *port);
    bool ClientDataChanged(class Client *client);

    bool operator==(const Universe &other) {
      return m_universe_id == other.UniverseId();
    }

    static const string K_UNIVERSE_NAME_VAR;
    static const string K_UNIVERSE_MODE_VAR;
    static const string K_UNIVERSE_PORT_VAR;
    static const string K_UNIVERSE_CLIENTS_VAR;
    static const string K_MERGE_HTP_STR;
    static const string K_MERGE_LTP_STR;

  private:
    Universe(const Universe&);
    Universe& operator=(const Universe&);
    bool UpdateDependants();
    void UpdateName();
    void UpdateMode();
    bool HTPMergeAllSources();

    string m_universe_name;
    unsigned int m_universe_id;
    string m_universe_id_str;
    enum merge_mode m_merge_mode;      // merge mode
    vector<class AbstractPort*> m_ports;       // ports patched to this universe
    vector<class Client*> m_clients;  // clients listening to this universe
    class UniverseStore *m_universe_store;

    DmxBuffer m_buffer;
    ExportMap *m_export_map;
};

} //lla
#endif
