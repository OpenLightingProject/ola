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
 * ArduinoDMXUSBWidget.h
 * The ArduinoDMXUSB Widget (based on DMX King Ultra DMX Pro code by Simon Newton).
 * This is similar to the Enttec Usb Pro, but it fetches the number of input
 * and output ports.
 * Copyright (C) 2019 Perry Naseck (DaAwesomeP)
 */

#ifndef PLUGINS_USBPRO_ArduinoDMXUSBWIDGET_H_
#define PLUGINS_USBPRO_ArduinoDMXUSBWIDGET_H_

#include <deque>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "plugins/usbpro/GenericUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

typedef struct {
  uint8_t out_ports;
  uint8_t in_ports;
} arduinodmxusb_extended_parameters;

typedef ola::SingleUseCallback2<void, bool, const arduinodmxusb_extended_parameters&>
  arduinodmxusb_extended_params_callback;

/*
 * An ArduinoDMXUSB Widget
 */
class ArduinoDMXUSBWidget: public GenericUsbProWidget {
 public:
    explicit ArduinoDMXUSBWidget(ola::io::ConnectedDescriptor *descriptor);
    ~ArduinoDMXUSBWidget() {}
    void Stop() override { GenericStop(); SpecificStop(); }

    bool SendDMX(const DmxBuffer &buffer) override;
    bool SendDMXPort(unsigned int port, const DmxBuffer &buffer);
    void SpecificStop();

    void GetExtendedParameters(arduinodmxusb_extended_params_callback *callback);

 private:
    std::deque<arduinodmxusb_extended_params_callback*>
        m_outstanding_extended_param_callbacks;

    void HandleExtendedParameters(const uint8_t *data, unsigned int length);
    bool SendDMXWithLabel(uint8_t label, const DmxBuffer &data);

    void HandleMessage(uint8_t label,
                       const uint8_t *data,
                       unsigned int length) override;

    static const uint8_t DMX_START_PORT = 100;
    static const uint8_t EXTENDED_PARAMETERS_LABEL = 53;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_ArduinoDMXUSBWIDGET_H_
