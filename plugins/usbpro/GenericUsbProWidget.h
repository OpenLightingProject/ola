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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * GenericUsbProWidget.h
 * This class implements a generic Usb Pro style widget, which can send and
 * receive DMX as well and get/set widget parameters.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_GENERICUSBPROWIDGET_H_
#define PLUGINS_USBPRO_GENERICUSBPROWIDGET_H_

#include <deque>
#include <string>
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/thread/SchedulerInterface.h"
#include "plugins/usbpro/BaseUsbProWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

typedef struct {
  uint8_t firmware;
  uint8_t firmware_high;
  uint8_t break_time;
  uint8_t mab_time;
  uint8_t rate;
} usb_pro_parameters;

typedef ola::SingleUseCallback2<void, bool, const usb_pro_parameters&>
  usb_pro_params_callback;


/*
 * An Generic DMX USB PRO Widget. This just handles the base functionality,
 * other features like RDM, multi-universe support etc. can be layered on top
 * of this.
 */
class GenericUsbProWidget: public BaseUsbProWidget {
  public:
    explicit GenericUsbProWidget(ola::io::ConnectedDescriptor *descriptor);
    ~GenericUsbProWidget();

    void SetDMXCallback(ola::Callback0<void> *callback);
    void GenericStop();

    virtual bool SendDMX(const DmxBuffer &buffer);
    bool ChangeToReceiveMode(bool change_only);
    const DmxBuffer &FetchDMX() const;

    void GetParameters(usb_pro_params_callback *callback);
    bool SetParameters(uint8_t break_time,
                       uint8_t mab_time,
                       uint8_t rate);

  protected:
    // child classes can intercept this.
    virtual void HandleMessage(uint8_t label,
                               const uint8_t *data,
                               unsigned int length);
    void HandleDMX(const uint8_t *data, unsigned int length);

    static const uint8_t RECEIVED_DMX_LABEL = 5;

  private:
    bool m_active;
    DmxBuffer m_input_buffer;
    ola::Callback0<void> *m_dmx_callback;
    std::deque<usb_pro_params_callback*> m_outstanding_param_callbacks;

    void HandleParameters(const uint8_t *data, unsigned int length);
    void HandleDMXDiff(const uint8_t *data, unsigned int length);

    static const uint8_t REPROGRAM_FIRMWARE_LABEL = 2;
    static const uint8_t PARAMETERS_LABEL = 3;
    static const uint8_t SET_PARAMETERS_LABEL = 4;
    static const uint8_t DMX_RX_MODE_LABEL = 8;
    static const uint8_t DMX_CHANGED_LABEL = 9;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_GENERICUSBPROWIDGET_H_
