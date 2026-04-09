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
 * RobeWidgetDetector.h
 * Runs the Robe discovery routine.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_ROBEWIDGETDETECTOR_H_
#define PLUGINS_USBPRO_ROBEWIDGETDETECTOR_H_

#include <ola/Callback.h>
#include <ola/rdm/UID.h>
#include <ola/thread/SchedulingExecutorInterface.h>
#include <memory>
#include <map>
#include <string>
#include "plugins/usbpro/BaseRobeWidget.h"
#include "plugins/usbpro/WidgetDetectorInterface.h"

namespace ola {
namespace plugin {
namespace usbpro {

/*
 * Contains information about the Robe USB device.
*/
class RobeWidgetInformation {
 public:
  RobeWidgetInformation()
      : uid(0, 0),
        hardware_version(0),
        software_version(0),
        eeprom_version(0) {
  }
  RobeWidgetInformation(const RobeWidgetInformation &other):
      uid(other.uid),
      hardware_version(other.hardware_version),
      software_version(other.software_version),
      eeprom_version(other.eeprom_version) {
  }

  ola::rdm::UID uid;
  uint8_t hardware_version;
  uint8_t software_version;
  uint8_t eeprom_version;
  void operator=(const RobeWidgetInformation& other) {
      uid = other.uid;
      hardware_version = other.hardware_version;
      software_version = other.software_version;
      eeprom_version = other.eeprom_version;
  }
};


/*
 * Handles widget discovery for Robe devices.
 */
class RobeWidgetDetector: public WidgetDetectorInterface {
 public:
  typedef ola::Callback2<void,
                         ola::io::ConnectedDescriptor*,
                         const RobeWidgetInformation*> SuccessHandler;
  typedef ola::Callback1<void,
                         ola::io::ConnectedDescriptor*> FailureHandler;

  RobeWidgetDetector(
      ola::thread::SchedulingExecutorInterface *scheduler,
      SuccessHandler *on_success,
      FailureHandler *on_failure,
      unsigned int timeout = 200);
  ~RobeWidgetDetector();

  bool Discover(ola::io::ConnectedDescriptor *descriptor);

 private:
  // Hold the discovery state for a widget
  class DiscoveryState {
   public:
    DiscoveryState():
      discovery_state(INFO_SENT),
      timeout_id(ola::thread::INVALID_TIMEOUT) {
    }
    ~DiscoveryState() {}

    typedef enum {
      INFO_SENT,
      UID_SENT,
    } widget_state;

    RobeWidgetInformation information;
    widget_state discovery_state;
    ola::thread::timeout_id timeout_id;
  };

  ola::thread::SchedulingExecutorInterface *m_scheduler;
  const unsigned int m_timeout_ms;
  std::auto_ptr<SuccessHandler> m_callback;
  std::auto_ptr<FailureHandler> m_failure_callback;

  typedef std::map<DispatchingRobeWidget*, DiscoveryState> WidgetStateMap;
  WidgetStateMap m_widgets;

  void HandleMessage(DispatchingRobeWidget *widget,
                     uint8_t label,
                     const uint8_t *data,
                     unsigned int length);
  void HandleInfoMessage(DispatchingRobeWidget *widget,
                         const uint8_t *data,
                         unsigned int length);
  void HandleUidMessage(DispatchingRobeWidget *widget,
                        const uint8_t *data,
                        unsigned int length);
  void WidgetRemoved(DispatchingRobeWidget *widget);
  void FailWidget(DispatchingRobeWidget *widget);
  void CleanupWidget(DispatchingRobeWidget *widget);
  void DispatchWidget(DispatchingRobeWidget *widget,
                      const RobeWidgetInformation *info);
  void RemoveTimeout(DiscoveryState *discovery_state);
  void SetupTimeout(DispatchingRobeWidget *widget,
                    DiscoveryState *discovery_state);
  bool IsUnlocked(const RobeWidgetInformation &info);

  static const uint32_t MODEL_MASK = 0xffff0000;
  static const uint32_t RUI_DEVICE_PREFIX = 0x01000000;
  static const uint32_t WTX_DEVICE_PREFIX = 0x02000000;
  // 0x14 is good, 0xe is bad. actual version is probably somewhere in
  // between.
  static const uint8_t RUI_MIN_UNLOCKED_SOFTWARE_VERSION = 0x14;
  // we need at least v 11 for decent RDM support
  static const uint8_t WTX_MIN_SOFTWARE_VERSION = 0x0b;
};
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola
#endif  // PLUGINS_USBPRO_ROBEWIDGETDETECTOR_H_
