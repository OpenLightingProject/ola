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
 * DiscoveryAgentTestHelper.h
 * Helper classes for the DiscoveryAgent test.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef COMMON_RDM_DISCOVERYAGENTTESTHELPER_H_
#define COMMON_RDM_DISCOVERYAGENTTESTHELPER_H_

#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/DiscoveryAgent.h"

using ola::rdm::UID;
using ola::rdm::UIDSet;
using ola::rdm::DiscoveryAgent;
using std::vector;


typedef std::vector<class MockResponderInterface*> ResponderList;


/**
 * The interface for MockResponders
 */
class MockResponderInterface {
  public:
    virtual ~MockResponderInterface() {}

    virtual const UID& GetUID() const = 0;
    virtual void UnMute() = 0;
    virtual bool Mute(const UID &uid) = 0;
    virtual bool FormResponse(const UID &upper,
                              const UID &lower,
                              uint8_t *data,
                              unsigned int length) const = 0;

    enum {DISCOVERY_RESPONSE_SIZE = 24 };
};


/**
 * A MockResponder.
 */
class MockResponder: public MockResponderInterface {
  public:
    explicit MockResponder(const UID &uid)
        : m_uid(uid),
          m_muted(false) {
    }

    const UID& GetUID() const { return m_uid; }
    void UnMute() { m_muted = false; }
    virtual bool Mute(const UID &uid) {
      if (m_uid == uid) {
        m_muted = true;
        return true;
      }
      return false;
    }
    bool FormResponse(const UID &lower,
                      const UID &upper,
                      uint8_t *data,
                      unsigned int length) const {
      if (!ShouldRespond(lower, upper))
        return false;

      uint16_t manufacturer_id = m_uid.ManufacturerId();
      uint32_t device_id = m_uid.DeviceId();

      CPPUNIT_ASSERT(length == DISCOVERY_RESPONSE_SIZE);
      for (unsigned int i = 0; i < 7; i++)
        data[i] |= 0xfe;
      data[7] |= 0xaa;

      uint16_t checksum = 0;
      // manufacturer_id
      OrAndChecksum(data, 8, (manufacturer_id >> 8) | 0xaa, &checksum);
      OrAndChecksum(data, 9, (manufacturer_id >> 8) | 0x55, &checksum);
      OrAndChecksum(data, 10, manufacturer_id | 0xaa, &checksum);
      OrAndChecksum(data, 11, manufacturer_id | 0x55, &checksum);

      // device id
      OrAndChecksum(data, 12, (device_id >> 24) | 0xaa, &checksum);
      OrAndChecksum(data, 13, (device_id >> 24) | 0x55, &checksum);
      OrAndChecksum(data, 14, (device_id >> 16) | 0xaa, &checksum);
      OrAndChecksum(data, 15, (device_id >> 16) | 0x55, &checksum);
      OrAndChecksum(data, 16, (device_id >> 8) | 0xaa, &checksum);
      OrAndChecksum(data, 17, (device_id >> 8) | 0x55, &checksum);
      OrAndChecksum(data, 18, device_id | 0xaa, &checksum);
      OrAndChecksum(data, 19, device_id | 0x55, &checksum);


      data[20] |= (checksum >> 8) | 0xaa;
      data[21] |= (checksum >> 8) | 0x55;
      data[22] |= checksum | 0xaa;
      data[23] |= checksum | 0x55;
      return true;
    }

  protected:
    virtual bool ShouldRespond(const UID &lower, const UID &upper) const {
      if (m_uid < lower || m_uid > upper)
        return false;

      if (m_muted)
        return false;
      return true;
    }

    UID m_uid;
    bool m_muted;

  private:

    void OrAndChecksum(uint8_t *data,
                       unsigned int offset,
                       uint8_t value,
                       uint16_t *checksum) const {
      data[offset] = value;
      *checksum += value;
    }
};


/**
 * A responder which stops responding once the manufacturer_id matches. This
 * simulates a responder with broken UID inequality handling.
 */
class BiPolarResponder: public MockResponder {
  public:
    explicit BiPolarResponder(const UID &uid)
        : MockResponder(uid) {
    }

  protected:
    bool ShouldRespond(const UID &lower, const UID &upper) const {
      if (m_uid < lower || m_uid > upper)
        return false;

      if (m_muted)
        return false;

      if (m_uid.ManufacturerId() == lower.ManufacturerId() &&
          m_uid.ManufacturerId() == upper.ManufacturerId())
        return false;
      return true;
    }
};


/**
 * A responder which doesn't honor mute.
 */
class ObnoxiousResponder: public MockResponder {
  public:
    explicit ObnoxiousResponder(const UID &uid)
        : MockResponder(uid) {
    }

  protected:
    bool ShouldRespond(const UID &lower, const UID &upper) const {
      if (m_uid < lower || m_uid > upper)
        return false;
      return true;
    }
};


/**
 * A responder that doesn't respond to a mute message.
 */
class NonMutingResponder: public MockResponder {
  public:
    explicit NonMutingResponder(const UID &uid)
        : MockResponder(uid) {
    }

    bool Mute(const UID&) { return false; }
};


/**
 * A proxy responder
 */
class ProxyResponder: public MockResponder {
  public:
    explicit ProxyResponder(const UID &uid,
                            const ResponderList &responders)
        : MockResponder(uid),
          m_responders(responders) {
    }

    ~ProxyResponder() {
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter)
        delete *iter;
    }

    void UnMute() {
      m_muted = false;
      // unmute everything behind this proxy
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter)
        (*iter)->UnMute();
    }

    bool Mute(const UID &uid) {
      bool r = MockResponder::Mute(uid);
      if (m_muted) {
        ResponderList::const_iterator iter = m_responders.begin();
        for (; iter != m_responders.end(); ++iter)
          r |= (*iter)->Mute(uid);
      }
      return r;
    }

    bool FormResponse(const UID &lower,
                      const UID &upper,
                      uint8_t *data,
                      unsigned int length) const {
      bool r = MockResponder::FormResponse(lower, upper, data, length);
      if (m_muted) {
        ResponderList::const_iterator iter = m_responders.begin();
        for (; iter != m_responders.end(); ++iter) {
          if (r)
            break;
          r |= (*iter)->FormResponse(lower, upper, data, length);
        }
      }
      return r;
    }

  private:
    ResponderList m_responders;
};


/**
 * A class which implements the DiscoveryTargetInterface
 */
class MockDiscoveryTarget: public ola::rdm::DiscoveryTargetInterface {
  public:
    explicit MockDiscoveryTarget(const ResponderList &responders)
        : m_responders(responders) {
    }

    ~MockDiscoveryTarget() {
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter)
        delete *iter;
    }

    // Mute a device
    void MuteDevice(const UID &target, MuteDeviceCallback *mute_complete) {
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter) {
        if ((*iter)->Mute(target)) {
          mute_complete->Run(true);
          return;
        }
      }
      // if we made it this far the responder has gone
      mute_complete->Run(false);
    }

    // Un Mute all devices
    void UnMuteAll(UnMuteDeviceCallback *unmute_complete) {
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter)
        (*iter)->UnMute();
      OLA_INFO << "un muted all";
      unmute_complete->Run();
    }

    // Send a branch request
    void Branch(const UID &lower, const UID &upper, BranchCallback *callback) {
      unsigned int data_size = MockResponder::DISCOVERY_RESPONSE_SIZE;
      uint8_t data[data_size];
      memset(data, 0, data_size);
      bool valid = false;
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter) {
        valid |= (*iter)->FormResponse(lower, upper, data, data_size);
      }

      /*
      OLA_INFO << "in branch (" << lower << ", " << upper << ") valid was  " <<
        valid;
      */
      if (valid)
        callback->Run(data, data_size);
      else
        callback->Run(NULL, 0);
    }

    // Add a responder to the list of responders
    void AddResponder(MockResponderInterface *responder) {
      m_responders.push_back(responder);
    }

    // Remove a responder from the list
    void RemoveResponder(const UID &uid) {
      ResponderList::iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter) {
        if ((*iter)->GetUID() == uid) {
          delete *iter;
          m_responders.erase(iter);
          break;
        }
      }
    }

  private:
    ResponderList m_responders;
};
#endif  // COMMON_RDM_DISCOVERYAGENTTESTHELPER_H_
