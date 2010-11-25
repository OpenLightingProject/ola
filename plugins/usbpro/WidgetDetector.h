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
 * WidgetDetector.h
 * Handles creating UsbWidget objects.
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_WIDGETDETECTOR_H_
#define PLUGINS_USBPRO_WIDGETDETECTOR_H_

#include <string.h>
#include <ola/network/SelectServerInterface.h>
#include <map>
#include <string>
#include "plugins/usbpro/UsbWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * Contains information about a usb device.
 */
class DeviceInformation {
  public:
    DeviceInformation():
        esta_id(0),
        device_id(0) {
      memset(serial, 0, SERIAL_LENGTH);
    }
    enum {SERIAL_LENGTH = 4};

    uint16_t esta_id;
    uint16_t device_id;
    string manufactuer;
    string device;
    uint8_t serial[SERIAL_LENGTH];
};


/*
 * Implement this to respond to widget discovery events
 */
class WidgetDetectorListener {
  public:
    virtual ~WidgetDetectorListener() {}

    /*
     * Ownership of the widget is transferred here
     */
    virtual void NewWidget(UsbWidget *widget,
                           const DeviceInformation &info) = 0;
};


/*
 * Handles widget discovery
 */
class WidgetDetector {
  public:
    explicit WidgetDetector(ola::network::SelectServerInterface *ss):
        m_ss(ss),
        m_timeout_id(ola::network::INVALID_TIMEOUT) {
    }
    ~WidgetDetector();

    void SetListener(WidgetDetectorListener *listener) {
      m_listener = listener;
    }
    bool Discover(const string &path);

    // called by the widgets
    void HandleMessage(UsbWidget *widget, uint8_t label, unsigned int length,
                       const uint8_t *data);
    void DiscoveryTimeout(UsbWidget *widget);
    int DeviceClosed(UsbWidget *widget);

  private:
    typedef struct {
      uint8_t id_low;
      uint8_t id_high;
      uint8_t text[32];
      uint8_t terminator;
    } id_response;

    WidgetDetectorListener *m_listener;
    ola::network::SelectServerInterface *m_ss;
    std::map<UsbWidget*, DeviceInformation> m_widgets;
    ola::network::timeout_id m_timeout_id;

    bool SendDiscoveryMessages(UsbWidget *widget);
    void HandleIdResponse(UsbWidget *widget, unsigned int length,
                          const uint8_t *data, bool is_device);
    void HandleSerialResponse(UsbWidget *widget, unsigned int length,
                              const uint8_t *data);
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_WIDGETDETECTOR_H_
