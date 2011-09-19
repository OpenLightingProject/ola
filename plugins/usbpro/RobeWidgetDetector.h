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
 * RobeWidgetDetector.h
 * Runs the Robe discovery routine.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ROBEWIDGETDETECTOR_H_
#define PLUGINS_USBPRO_ROBEWIDGETDETECTOR_H_

#include <ola/Callback.h>
#include <ola/thread/SchedulingExecutorInterface.h>
#include <map>
#include <string>
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/WidgetDetectorInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {


/*
 * Contains information about the Robe USB device.
*/
typedef struct {
  uint8_t hardware_version;
  uint8_t software_version;
  uint8_t eeprom_version;
} RobeWidgetInformation;


/*
 * Handles widget discovery for Robe devices.
 */
class RobeWidgetDetector: public WidgetDetectorInterface {
  public:
    typedef ola::Callback2<void,
                           RobeWidget*,
                           const RobeWidgetInformation*> SuccessHandler;
    typedef ola::Callback1<void,
                           ola::network::ConnectedDescriptor*> FailureHandler;

    RobeWidgetDetector(
        ola::thread::SchedulingExecutorInterface *scheduler,
        SuccessHandler *on_success,
        FailureHandler *on_failure,
        unsigned int timeout = 200);
    ~RobeWidgetDetector();

    bool Discover(ola::network::ConnectedDescriptor *descriptor);

  private:
    typedef struct {
      uint8_t hardware_version;
      uint8_t software_version;
      uint8_t eeprom_version;
      uint8_t empty;
      uint8_t empty2;
    } info_response_t;

    ola::thread::SchedulingExecutorInterface *m_scheduler;
    const unsigned int m_timeout_ms;
    SuccessHandler *m_callback;
    FailureHandler *m_failure_callback;

    typedef std::map<RobeWidget*, ola::thread::timeout_id> WidgetTimeoutMap;
    WidgetTimeoutMap m_widgets;

    void HandleMessage(RobeWidget *widget,
                       uint8_t label,
                       const uint8_t *data,
                       unsigned int length);
    void WidgetRemoved(RobeWidget *widget);
    void FailWidget(RobeWidget *widget);
    void CleanupWidget(RobeWidget *widget);
    void DispatchWidget(RobeWidget *widget, const RobeWidgetInformation *info);
};
}  // usbpro
}  // plugin
}  // ola
#endif  // PLUGINS_USBPRO_ROBEWIDGETDETECTOR_H_
