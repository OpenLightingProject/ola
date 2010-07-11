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
 * DmxTriDevice.h
 * The Jese DMX-TRI device.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_DMXTRIDEVICE_H_
#define PLUGINS_USBPRO_DMXTRIDEVICE_H_

#include <map>
#include <string>
#include <queue>
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/UsbDevice.h"
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

using std::queue;


/*
 * An DMX TRI Device
 */
class DmxTriDevice: public UsbDevice, public WidgetListener {
  public:
    DmxTriDevice(const ola::PluginAdaptor *plugin_adaptor,
                 ola::AbstractPlugin *owner,
                 const string &name,
                 UsbWidget *widget,
                 uint16_t esta_id,
                 uint16_t device_id,
                 uint32_t serial);
    ~DmxTriDevice();

    string DeviceId() const { return m_device_id; }
    bool StartHook();
    void PrePortStop();
    bool SendDMX(const DmxBuffer &buffer) const;

    bool HandleRDMRequest(const ola::rdm::RDMRequest *request);
    void RunRDMDiscovery();

    bool CheckDiscoveryStatus();

    void HandleMessage(UsbWidget* widget,
                       uint8_t label,
                       unsigned int length,
                       const uint8_t *data);

  private:
    string m_device_id;
    const PluginAdaptor *m_plugin_adaptor;
    timeout_id m_rdm_timeout_id;
    map<const UID, uint8_t> m_uid_index_map;
    unsigned int m_uid_count;
    queue<const class ola::rdm::RDMRequest *> m_pending_requests;
    bool m_rdm_request_pending;
    uint16_t m_last_esta_id;

    bool InDiscoveryMode() const;
    bool SendDiscoveryStart();
    bool SendDiscoveryStat();
    void FetchNextUID();
    bool SendSetFilter(uint16_t esta_id);
    void MaybeSendRDMRequest();
    void DispatchNextRequest();
    void DispatchQueuedGet(const ola::rdm::RDMRequest* request);
    void StopDiscovery();

    void HandleDiscoveryAutoResponse(uint8_t return_code,
                                     const uint8_t *data,
                                     unsigned int length);
    void HandleDiscoverStatResponse(uint8_t return_code,
                                    const uint8_t *data,
                                    unsigned int length);
    void HandleRemoteUIDResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);
    void HandleRemoteRDMResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);
    void HandleQueuedGetResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);
    void HandleSetFilterResponse(uint8_t return_code,
                                 const uint8_t *data,
                                 unsigned int length);

    typedef enum {
      EC_NO_ERROR = 0,
      EC_CONSTRAINT = 1,
      EC_UNKNOWN_COMMAND = 2,
      EC_INVALID_OPTION = 3,
      EC_FRAME_FORMAT = 4,
      EC_DATA_TOO_LONG = 5,
      EC_DATA_MISSING = 6,
      EC_SYSTEM_MODE = 7,
      EC_SYSTEM_BUSY = 8,
      EC_DATA_CHECKSUM = 0x0a,
      EC_INCOMPATIBLE = 0x0b,
      EC_RESPONSE_TIME = 0x10,
      EC_RESPONSE_WAIT = 0x11,
      EC_RESPONSE_MORE = 0x12,
      EC_RESPONSE_TRANSACTION = 0x13,
      EC_RESPONSE_SUBDEVICE = 0x14,
      EC_RESPONSE_FORMAT = 0x15,
      EC_RESPONSE_CHECKSUM = 0x16,
      EC_RESPONSE_NONE = 0x18,
      EC_RESPONSE_IDENTITY = 0x1a,
      EC_RESPONSE_MUTE = 0x1b,
      EC_RESPONSE_DISCOVERY = 0x1c,
      EC_RESPONSE_UNEXPECTED = 0x1d,
      EC_UNKNOWN_PID = 0x20,
      EC_FORMAT_ERROR = 0x21,
      EC_HARDWARE_FAULT = 0x22,
      EC_PROXY_REJECT = 0x23,
      EC_WRITE_PROTECT = 0x24,
      EC_UNSUPPORTED_COMMAND_CLASS = 0x25,
      EC_OUT_OF_RANGE = 0x26,
      EC_BUFFER_FULL = 0x27,
      EC_FRAME_OVERFLOW = 0x28,
      EC_SUBDEVICE_UNKNOWN = 0x29,
    } dmx_tri_error_codes;

    static const unsigned int DATA_OFFSET = 2;  // first two bytes are CI & RC
    static const uint8_t EXTENDED_COMMAND_LABEL = 88;  // 'X'
    static const uint8_t DISCOVER_AUTO_COMMAND_ID = 0x33;
    static const uint8_t DISCOVER_STATUS_COMMAND_ID = 0x34;
    static const uint8_t REMOTE_UID_COMMAND_ID = 0x35;
    static const uint8_t REMOTE_GET_COMMAND_ID = 0x38;
    static const uint8_t REMOTE_SET_COMMAND_ID = 0x39;
    static const uint8_t QUEUED_GET_COMMAND_ID = 0x3a;
    static const uint8_t SET_FILTER_COMMAND_ID = 0x3d;
    // The ms delay between checking on the RDM discovery process
    static const unsigned int RDM_STATUS_INTERVAL_MS = 100;
};


/*
 * A single output port per device
 */
class DmxTriOutputPort: public BasicOutputPort {
  public:
    explicit DmxTriOutputPort(DmxTriDevice *parent)
        : BasicOutputPort(parent, 0),
          m_device(parent) {}

    bool WriteDMX(const DmxBuffer &buffer, uint8_t priority) {
      return m_device->SendDMX(buffer);
      (void) priority;
    }
    string Description() const { return ""; }

    bool HandleRDMRequest(const ola::rdm::RDMRequest *request) {
      return m_device->HandleRDMRequest(request);
    }

    void RunRDMDiscovery() {
      m_device->RunRDMDiscovery();
    }

  private:
    DmxTriDevice *m_device;
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_DMXTRIDEVICE_H_
