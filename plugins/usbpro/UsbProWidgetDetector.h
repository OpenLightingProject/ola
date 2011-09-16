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
 * UsbProWidgetDetector.h
 * Performs the Usb Pro style discovery routine. See the
 * UsbProWidgetDetector.cpp for details.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_USBPROWIDGETDETECTOR_H_
#define PLUGINS_USBPRO_USBPROWIDGETDETECTOR_H_

#include <ola/network/SelectServerInterface.h>
#include <map>
#include <string>
#include "ola/Callback.h"
#include "plugins/usbpro/WidgetDetectorInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {

class BaseUsbProWidget;

/*
 * Contains information about a USB Pro like device.
 */
class UsbProWidgetInformation {
  public:
    UsbProWidgetInformation():
        esta_id(0),
        device_id(0),
        serial(0) {
    }
    UsbProWidgetInformation(const UsbProWidgetInformation &other):
        esta_id(other.esta_id),
        device_id(other.device_id),
        serial(other.serial),
        manufactuer(other.manufactuer),
        device(other.device) {
    }
    UsbProWidgetInformation& operator=(const UsbProWidgetInformation &other);
    enum {SERIAL_LENGTH = 4};

    uint16_t esta_id;
    uint16_t device_id;
    uint32_t serial;
    string manufactuer;
    string device;
};


/*
 * Handles the discovery routine for devices that behave like a Enttec Usb Pro.
 */
class UsbProWidgetDetector: public WidgetDetectorInterface {
  public:
    typedef ola::Callback2<void,
                           BaseUsbProWidget*,
                           const UsbProWidgetInformation*> SuccessHandler;
    typedef ola::Callback1<void,
                           ola::network::ConnectedDescriptor*> FailureHandler;

    UsbProWidgetDetector(
        ola::network::SelectServerInterface *ss,
        SuccessHandler *on_success,
        FailureHandler *on_failure,
        unsigned int message_interval = 200);
    ~UsbProWidgetDetector();

    bool Discover(ola::network::ConnectedDescriptor *descriptor);

  private:
    typedef struct {
      uint8_t id_low;
      uint8_t id_high;
      char text[32];
      uint8_t terminator;
    } id_response;

    // Hold the discovery state for a widget
    class DiscoveryState {
      public:
        DiscoveryState():
          discovery_state(MANUFACTURER_SENT),
          timeout_id(ola::network::INVALID_TIMEOUT) {
        }
        ~DiscoveryState() {}

        typedef enum {
          MANUFACTURER_SENT,
          DEVICE_SENT,
          SERIAL_SENT,
        } widget_state;

        UsbProWidgetInformation information;
        widget_state discovery_state;
        ola::network::timeout_id timeout_id;
    };

    ola::network::SelectServerInterface *m_ss;
    SuccessHandler *m_callback;
    FailureHandler *m_failure_callback;

    typedef std::map<BaseUsbProWidget*, DiscoveryState> WidgetStateMap;
    WidgetStateMap m_widgets;
    unsigned int m_timeout_ms;

    void HandleMessage(BaseUsbProWidget *widget,
                       uint8_t label,
                       const uint8_t *data,
                       unsigned int length);
    void WidgetRemoved(BaseUsbProWidget *widget);
    void SetupTimeout(BaseUsbProWidget *widget,
                      DiscoveryState *discovery_state);
    void RemoveTimeout(DiscoveryState *discovery_state);
    void SendNameRequest(BaseUsbProWidget *widget);
    void SendSerialRequest(BaseUsbProWidget *widget);
    void DiscoveryTimeout(BaseUsbProWidget *widget);
    void HandleIdResponse(BaseUsbProWidget *widget,
                          unsigned int length,
                          const uint8_t *data,
                          bool is_device);
    void HandleSerialResponse(BaseUsbProWidget *widget,
                              unsigned int length,
                              const uint8_t *data);
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPROWIDGETDETECTOR_H_
