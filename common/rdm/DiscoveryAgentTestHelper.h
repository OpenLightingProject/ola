/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * DiscoveryAgentTestHelper.h
 * Helper classes for the DiscoveryAgent test.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef COMMON_RDM_DISCOVERYAGENTTESTHELPER_H_
#define COMMON_RDM_DISCOVERYAGENTTESTHELPER_H_

#include <cppunit/extensions/HelperMacros.h>
#include <string.h>
#include <algorithm>
#include <vector>

#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/rdm/UIDSet.h"
#include "ola/rdm/DiscoveryAgent.h"


typedef std::vector<class MockResponderInterface*> ResponderList;


/**
 * The interface for MockResponders
 */
class MockResponderInterface {
 public:
    virtual ~MockResponderInterface() {}

    virtual const ola::rdm::UID& GetUID() const = 0;
    virtual void UnMute() = 0;
    virtual bool Mute(const ola::rdm::UID &uid) = 0;
    virtual bool FormResponse(const ola::rdm::UID &upper,
                              const ola::rdm::UID &lower,
                              uint8_t *data,
                              unsigned int *length) const = 0;

    enum {DISCOVERY_RESPONSE_SIZE = 24 };
};


/**
 * A MockResponder.
 */
class MockResponder: public MockResponderInterface {
 public:
    explicit MockResponder(const ola::rdm::UID &uid)
        : m_uid(uid),
          m_muted(false) {
    }

    const ola::rdm::UID& GetUID() const { return m_uid; }
    void UnMute() { m_muted = false; }

    virtual bool Mute(const ola::rdm::UID &uid) {
      if (m_uid == uid) {
        m_muted = true;
        return true;
      }
      return false;
    }

    bool FormResponse(const ola::rdm::UID &lower,
                      const ola::rdm::UID &upper,
                      uint8_t *data,
                      unsigned int *length) const {
      if (!ShouldRespond(lower, upper))
        return false;

      uint16_t manufacturer_id = m_uid.ManufacturerId();
      uint32_t device_id = m_uid.DeviceId();

      CPPUNIT_ASSERT(*length >= DISCOVERY_RESPONSE_SIZE);
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
      *length = DISCOVERY_RESPONSE_SIZE;
      return true;
    }

 protected:
    virtual bool ShouldRespond(const ola::rdm::UID &lower,
                               const ola::rdm::UID &upper) const {
      if (m_uid < lower || m_uid > upper)
        return false;

      if (m_muted)
        return false;
      return true;
    }

    ola::rdm::UID m_uid;
    bool m_muted;

 private:
    void OrAndChecksum(uint8_t *data,
                       unsigned int offset,
                       uint8_t value,
                       uint16_t *checksum) const {
      data[offset] |= value;
      *checksum += value;
    }
};


/**
 * A responder which stops responding once the manufacturer_id matches. This
 * simulates a responder with broken UID inequality handling.
 */
class BiPolarResponder: public MockResponder {
 public:
    explicit BiPolarResponder(const ola::rdm::UID &uid)
        : MockResponder(uid) {
    }

 protected:
    bool ShouldRespond(const ola::rdm::UID &lower,
                       const ola::rdm::UID &upper) const {
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
    explicit ObnoxiousResponder(const ola::rdm::UID &uid)
        : MockResponder(uid) {
    }

 protected:
    bool ShouldRespond(const ola::rdm::UID &lower,
                       const ola::rdm::UID &upper) const {
      if (m_uid < lower || m_uid > upper)
        return false;
      return true;
    }
};


/**
 * A responder which replies to DUB with extra data
 */
class RamblingResponder: public MockResponder {
 public:
    explicit RamblingResponder(const ola::rdm::UID &uid)
        : MockResponder(uid) {
    }

    bool FormResponse(const ola::rdm::UID &lower,
                      const ola::rdm::UID &upper,
                      uint8_t *data,
                      unsigned int *length) const {
      unsigned int data_size = *length;
      bool ok = MockResponder::FormResponse(lower, upper, data, length);
      if (ok && data_size > DISCOVERY_RESPONSE_SIZE) {
        // add some random data and increase the packet size
        data[DISCOVERY_RESPONSE_SIZE] = 0x52;
        (*length)++;
      }
      return ok;
  }
};


/**
 * A responder which replies to DUB with too little data
 */
class BriefResponder: public MockResponder {
 public:
    explicit BriefResponder(const ola::rdm::UID &uid)
        : MockResponder(uid) {
    }

    bool FormResponse(const ola::rdm::UID &lower,
                      const ola::rdm::UID &upper,
                      uint8_t *data,
                      unsigned int *length) const {
      bool ok = MockResponder::FormResponse(lower, upper, data, length);
      if (ok && *length > 1)
        (*length)--;
      return ok;
  }
};


/**
 * A responder that doesn't respond to a mute message.
 */
class NonMutingResponder: public MockResponder {
 public:
    explicit NonMutingResponder(const ola::rdm::UID &uid)
        : MockResponder(uid) {
    }

    bool Mute(const ola::rdm::UID&) { return false; }
};


/**
 * A responder that only mutes after N attempts
 */
class FlakeyMutingResponder: public MockResponder {
 public:
    explicit FlakeyMutingResponder(const ola::rdm::UID &uid,
                                  unsigned int threshold = 2)
        : MockResponder(uid),
          m_threshold(threshold),
          m_attempts(0) {
    }

    bool Mute(const ola::rdm::UID &uid) {
      if (m_uid != uid)
        return false;

      m_attempts++;
      if (m_attempts > m_threshold) {
        m_muted = true;
        return true;
      }
      return false;
    }

    void Reset() { m_attempts = 0; }

 private:
    unsigned int m_threshold;
    unsigned int m_attempts;
};


/**
 * A proxy responder
 */
class ProxyResponder: public MockResponder {
 public:
    explicit ProxyResponder(const ola::rdm::UID &uid,
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

    bool Mute(const ola::rdm::UID &uid) {
      bool r = MockResponder::Mute(uid);
      if (m_muted) {
        ResponderList::const_iterator iter = m_responders.begin();
        for (; iter != m_responders.end(); ++iter)
          r |= (*iter)->Mute(uid);
      }
      return r;
    }

    bool FormResponse(const ola::rdm::UID &lower,
                      const ola::rdm::UID &upper,
                      uint8_t *data,
                      unsigned int *length) const {
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
        : m_responders(responders),
          m_unmute_calls(0) {
    }

    ~MockDiscoveryTarget() {
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter)
        delete *iter;
    }

    void ResetCounters() {
      m_unmute_calls = 0;
    }

    unsigned int UnmuteCallCount() const {
      return m_unmute_calls;
    }

    // Mute a device
    void MuteDevice(const ola::rdm::UID &target,
                    MuteDeviceCallback *mute_complete) {
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
      for (; iter != m_responders.end(); ++iter) {
        (*iter)->UnMute();
      }
      m_unmute_calls++;
      unmute_complete->Run();
    }

    // Send a branch request
    void Branch(const ola::rdm::UID &lower,
                const ola::rdm::UID &upper,
                BranchCallback *callback) {
      // alloc twice the amount we need
      unsigned int data_size = 2 * MockResponder::DISCOVERY_RESPONSE_SIZE;
      uint8_t data[data_size];
      memset(data, 0, data_size);
      bool valid = false;
      unsigned int actual_size = 0;
      ResponderList::const_iterator iter = m_responders.begin();
      for (; iter != m_responders.end(); ++iter) {
        unsigned int data_used = data_size;
        if ((*iter)->FormResponse(lower, upper, data, &data_used)) {
          actual_size = std::max(data_used, actual_size);
          valid = true;
        }
      }

      if (valid)
        callback->Run(data, actual_size);
      else
        callback->Run(NULL, 0);
    }

    // Add a responder to the list of responders
    void AddResponder(MockResponderInterface *responder) {
      m_responders.push_back(responder);
    }

    // Remove a responder from the list
    void RemoveResponder(const ola::rdm::UID &uid) {
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
    unsigned int m_unmute_calls;
};
#endif  // COMMON_RDM_DISCOVERYAGENTTESTHELPER_H_
