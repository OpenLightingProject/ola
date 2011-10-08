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

using std::auto_ptr;

class DispatchingUsbProWidget;

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
        manufacturer(other.manufacturer),
        device(other.device) {
    }
    UsbProWidgetInformation& operator=(const UsbProWidgetInformation &other);
    enum {SERIAL_LENGTH = 4};

    uint16_t esta_id;
    uint16_t device_id;
    uint32_t serial;
    string manufacturer;
    string device;
};


/*
 * Handles the discovery routine for devices that behave like a Enttec Usb Pro.
 */
class UsbProWidgetDetector: public WidgetDetectorInterface {
  public:
    typedef ola::Callback2<void,
                           ola::network::ConnectedDescriptor*,
                           const UsbProWidgetInformation*> SuccessHandler;
    typedef ola::Callback1<void,
                           ola::network::ConnectedDescriptor*> FailureHandler;

    UsbProWidgetDetector(
        ola::thread::SchedulingExecutorInterface *scheduler,
        SuccessHandler *on_success,
        FailureHandler *on_failure,
        unsigned int message_interval = 200);
    ~UsbProWidgetDetector();

    bool Discover(ola::network::ConnectedDescriptor *descriptor);

  private:
    // Hold the discovery state for a widget
    class DiscoveryState {
      public:
        DiscoveryState():
          discovery_state(MANUFACTURER_SENT),
          timeout_id(ola::thread::INVALID_TIMEOUT) {
        }
        ~DiscoveryState() {}

        typedef enum {
          MANUFACTURER_SENT,
          DEVICE_SENT,
          SERIAL_SENT,
        } widget_state;

        UsbProWidgetInformation information;
        widget_state discovery_state;
        ola::thread::timeout_id timeout_id;
    };

    ola::thread::SchedulingExecutorInterface *m_scheduler;
    auto_ptr<SuccessHandler> m_callback;
    auto_ptr<FailureHandler> m_failure_callback;

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
    void DiscoveryTimeout(DispatchingUsbProWidget *widget);
    void HandleIdResponse(DispatchingUsbProWidget *widget,
                          unsigned int length,
                          const uint8_t *data,
                          bool is_device);
    void HandleSerialResponse(DispatchingUsbProWidget *widget,
                              unsigned int length,
                              const uint8_t *data);
    void DispatchWidget(DispatchingUsbProWidget *widget,
                        const UsbProWidgetInformation *info);
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_USBPROWIDGETDETECTOR_H_
