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
 * PidStore.h
 * Holds information abouts RDM PIDs.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_RDM_PIDSTORE_H_
#define INCLUDE_OLA_RDM_PIDSTORE_H_

#include <stdint.h>
#include <ola/messaging/Descriptor.h>
#include <istream>
#include <map>
#include <string>
#include <vector>

namespace ola {
namespace rdm {

using std::map;
using std::vector;
using std::string;
using ola::messaging::Descriptor;

class PidStore;
class PidDescriptor;

/**
 * The root PID Store.
 */
class RootPidStore {
  public:
    typedef map<uint16_t, const PidStore*> ManufacturerMap;

    RootPidStore(const PidStore *esta_store,
                 const ManufacturerMap &manufacturer_stores,
                 uint64_t version = 0)
        : m_esta_store(esta_store),
          m_manufacturer_store(manufacturer_stores),
          m_version(version) {
    }
    ~RootPidStore();

    // Seconds since epoch in UTC
    uint64_t Version() const { return m_version; }

    // This holds the ESTA PIDs
    const PidStore *EstaStore() const {
      return m_esta_store;
    }

    // Holds Manufacturer PID data
    const PidStore *ManufacturerStore(uint16_t esta_id) const;

    // Load a RootPidStore from a file
    static const RootPidStore *LoadFromFile(const std::string &file,
                                            bool validate = true);

  private:
    const PidStore *m_esta_store;
    ManufacturerMap m_manufacturer_store;
    uint64_t m_version;

    RootPidStore(const RootPidStore&);
    RootPidStore& operator=(const RootPidStore&);
    void CleanStore();
};


/**
 * Stores the PidDescriptors for a set of PIDs in a common namespace.
 */
class PidStore {
  public:
    explicit PidStore(const vector<const PidDescriptor*> &pids);
    ~PidStore();

    unsigned int PidCount() const { return m_pid_by_value.size(); }
    void AllPids(vector<const PidDescriptor*> *pids) const;
    const PidDescriptor *LookupPID(uint16_t pid_value) const;
    const PidDescriptor *LookupPID(const string &pid_name) const;

  private:
    typedef map<uint16_t, const PidDescriptor*> PidMap;
    typedef map<string, const PidDescriptor*> PidNameMap;
    PidMap m_pid_by_value;
    PidNameMap m_pid_by_name;

    PidStore(const PidStore&);
    PidStore& operator=(const PidStore&);
};


/**
 * Contains the descriptors for the GET/SET Requests & Responses for a single
 * PID.
 */
class PidDescriptor {
  public:
    // TODO(simon): use the enums from the Pids.proto instead of duplicating
    // here.
    typedef enum {
      ROOT_DEVICE,  // 0 only
      ANY_SUB_DEVICE,  // 0 - 512 or ALL_DEVICES
      NON_BROADCAST_SUB_DEVICE,  // 0 - 512
      SPECIFIC_SUB_DEVICE,  // 1- 512
    } sub_device_valiator;

    PidDescriptor(const string &name,
                  uint16_t value,
                  const Descriptor *get_request,
                  const Descriptor *get_response,
                  const Descriptor *set_request,
                  const Descriptor *set_response,
                  sub_device_valiator get_sub_device_range,
                  sub_device_valiator set_sub_device_range)
        : m_name(name),
          m_pid_value(value),
          m_get_request(get_request),
          m_get_response(get_response),
          m_set_request(set_request),
          m_set_response(set_response),
          m_get_subdevice_range(get_sub_device_range),
          m_set_subdevice_range(set_sub_device_range) {
    }
    ~PidDescriptor();

    const string &Name() const { return m_name; }
    uint16_t Value() const { return m_pid_value; }
    const Descriptor *GetRequest() const { return m_get_request; }
    const Descriptor *GetResponse() const { return m_get_response; }
    const Descriptor *SetRequest() const { return m_set_request; }
    const Descriptor *SetResponse() const { return m_set_response; }

    bool IsGetValid(uint16_t sub_device) const;
    bool IsSetValid(uint16_t sub_device) const;

  private:
    const string m_name;
    uint16_t m_pid_value;
    const Descriptor *m_get_request;
    const Descriptor *m_get_response;
    const Descriptor *m_set_request;
    const Descriptor *m_set_response;
    sub_device_valiator m_get_subdevice_range;
    sub_device_valiator m_set_subdevice_range;

    PidDescriptor(const PidDescriptor&);
    PidDescriptor& operator=(const PidDescriptor&);

    bool RequestValid(uint16_t sub_device,
                      const sub_device_valiator &validator) const;
};
}  // rdm
}  // ola
#endif  // INCLUDE_OLA_RDM_PIDSTORE_H_
