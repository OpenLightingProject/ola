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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * UsbProWidgetDetector.h
 * Handles the discovery process for widgets that implement the Usb Pro frame
 * format.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPROWIDGETDETECTOR_H_
#define PLUGINS_USBPRO_USBPROWIDGETDETECTOR_H_

#include <ola/thread/SchedulingExecutorInterface.h>
#include <map>
#include <memory>
#include <string>
#include "ola/Callback.h"
#include "plugins/usbpro/WidgetDetectorInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {

class DispatchingUsbProWidget;

/*
 * Contains information about a USB Pro like device.
 */
class UsbProWidgetInformation {
 public:
  typedef uint32_t DeviceSerialNumber;
  typedef uint16_t DeviceFirmwareVersion;

  UsbProWidgetInformation():
      esta_id(0),
      device_id(0),
      serial(0),
      firmware_version(0),
      has_firmware_version(false),
      dual_port(false) {
  }
  UsbProWidgetInformation(const UsbProWidgetInformation &other):
      esta_id(other.esta_id),
      device_id(other.device_id),
      serial(other.serial),
      firmware_version(other.firmware_version),
      has_firmware_version(other.has_firmware_version),
      manufacturer(other.manufacturer),
      device(other.device),
      dual_port(other.dual_port) {
  }
  UsbProWidgetInformation& operator=(const UsbProWidgetInformation &other);

  void SetFirmware(DeviceFirmwareVersion new_firmware_version) {
    has_firmware_version = true;
    firmware_version = new_firmware_version;
  }

  enum {SERIAL_LENGTH = 4};

  uint16_t esta_id;
  uint16_t device_id;
  DeviceSerialNumber serial;
  DeviceFirmwareVersion firmware_version;
  bool has_firmware_version;
  std::string manufacturer;
  std::string device;
  bool dual_port;
};


/*
 * Handles the discovery routine for devices that behave like a Enttec Usb Pro.
 */
class UsbProWidgetDetector: public WidgetDetectorInterface {
 public:
  typedef ola::Callback2<void, ola::io::ConnectedDescriptor*,
                         const UsbProWidgetInformation*> SuccessHandler;
  typedef ola::Callback1<void, ola::io::ConnectedDescriptor*> FailureHandler;

  UsbProWidgetDetector(ola::thread::SchedulingExecutorInterface *scheduler,
                       SuccessHandler *on_success,
                       FailureHandler *on_failure,
                       unsigned int message_interval = 200);
  ~UsbProWidgetDetector();

  bool Discover(ola::io::ConnectedDescriptor *descriptor);

 private:
  // Hold the discovery state for a widget
  class DiscoveryState {
   public:
    DiscoveryState():
      discovery_state(MANUFACTURER_SENT),
      timeout_id(ola::thread::INVALID_TIMEOUT),
      sniffer_packets(0),
      hardware_version(0) {
    }
    ~DiscoveryState() {}

    typedef enum {
      MANUFACTURER_SENT,
      DEVICE_SENT,
      SERIAL_SENT,
      GET_PARAM_SENT,
      HARDWARE_VERSION_SENT,
    } widget_state;

    UsbProWidgetInformation information;
    widget_state discovery_state;
    ola::thread::timeout_id timeout_id;
    unsigned int sniffer_packets;
    uint8_t hardware_version;
  };

  ola::thread::SchedulingExecutorInterface *m_scheduler;
  const std::auto_ptr<SuccessHandler> m_callback;
  const std::auto_ptr<FailureHandler> m_failure_callback;

  typedef std::map<DispatchingUsbProWidget*, DiscoveryState> WidgetStateMap;
  WidgetStateMap m_widgets;
  unsigned int m_timeout_ms;

  void HandleMessage(DispatchingUsbProWidget *widget,
                     uint8_t label,
                     const uint8_t *data,
                     unsigned int length);
  void WidgetRemoved(DispatchingUsbProWidget *widget);
  void SetupTimeout(DispatchingUsbProWidget *widget,
                    DiscoveryState *discovery_state);
  void RemoveTimeout(DiscoveryState *discovery_state);
  void SendNameRequest(DispatchingUsbProWidget *widget);
  void SendSerialRequest(DispatchingUsbProWidget *widget);
  void SendGetParams(DispatchingUsbProWidget *widget);
  void MaybeSendHardwareVersionRequest(DispatchingUsbProWidget *widget);
  void SendAPIRequest(DispatchingUsbProWidget *widget);
  void DiscoveryTimeout(DispatchingUsbProWidget *widget);
  void HandleIdResponse(DispatchingUsbProWidget *widget,
                        unsigned int length,
                        const uint8_t *data,
                        bool is_device);
  void HandleSerialResponse(DispatchingUsbProWidget *widget,
                            unsigned int length,
                            const uint8_t *data);
  void HandleGetParams(DispatchingUsbProWidget *widget,
                       unsigned int length,
                       const uint8_t *data);
  void HandleHardwareVersionResponse(DispatchingUsbProWidget *widget,
                                     unsigned int length,
                                     const uint8_t *data);
  void HandleSnifferPacket(DispatchingUsbProWidget *widget);
  void CompleteWidgetDiscovery(DispatchingUsbProWidget *widget);
  void DispatchWidget(DispatchingUsbProWidget *widget,
                      const UsbProWidgetInformation *info);
  void HandleSniffer(DispatchingUsbProWidget *widget);

  static const uint8_t ENTTEC_SNIFFER_LABEL = 0x81;
  static const uint8_t USB_PRO_MKII_API_LABEL = 13;
  static const uint8_t DMX_PRO_MKII_VERSION = 2;
  // The API key associated with OLA
  static const uint32_t USB_PRO_MKII_API_KEY = 0x0d11b2d7;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_USBPROWIDGETDETECTOR_H_
