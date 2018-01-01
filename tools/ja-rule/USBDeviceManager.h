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
 * USBDeviceManager.h
 * Handles auto-detection of Ja Rule widgets.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef TOOLS_JA_RULE_USBDEVICEMANAGER_H_
#define TOOLS_JA_RULE_USBDEVICEMANAGER_H_

#include <libusb.h>

#include <ola/Callback.h>
#include <ola/io/SelectServer.h>
#include <ola/thread/ExecutorThread.h>
#include <ola/thread/Future.h>
#include <ola/thread/Thread.h>

#include <map>
#include <memory>
#include <utility>

#include "libs/usb/HotplugAgent.h"
#include "libs/usb/JaRuleWidget.h"
#include "libs/usb/Types.h"

/**
 * @brief Manages adding / removing Open Lighting Devices.
 *
 * As Open Lighting Widgets are added or removed, this executes the
 * callback to notify the recipient.
 */
class USBDeviceManager {
 public:
  enum EventType {
    WIDGET_ADDED,   //!< The widget was added.
    WIDGET_REMOVED,   //!< The widget ws removed.
  };

  /**
   * @brief Indicates a device has been added or removed
   */
  typedef ola::Callback2<void, EventType, ola::usb::JaRuleWidget*>
      NotificationCallback;

  /**
   * @brief Create a new USBDeviceManager.
   * @param ss The executor to run the notification_cb on.
   * @param notification_cb The callback to run when the widget is added or
   *   removed. Ownership is transferred.
   */
  USBDeviceManager(ola::io::SelectServer* ss,
                   NotificationCallback* notification_cb);

  /**
   * @brief Destructor.
   *
   * This will stop the USBDeviceManager if it's still running.
   */
  ~USBDeviceManager();

  /**
   * @brief Get the AsynchronousLibUsbAdaptor to use.
   * @returns An AsynchronousLibUsbAdaptor, ownership is not transferred.
   * @pre Must be called after Start()
   *
   * The adaptor is valid until the call to Stop().
   */
  ola::usb::AsynchronousLibUsbAdaptor *GetUSBAdaptor() const;

  /**
   * @brief Start the device manager.
   * @returns true if the manager started correctly, false otherwise.
   */
  bool Start();

  /**
   * @brief Stop the device manager.
   * @returns true if stopped correctly, false otherwise.
   *
   * Stop() may result in notifications being run, however once Stop() returns,
   * no further calls to the notification callback will be made.
   */
  bool Stop();

  /**
   * @brief Called by the HotplugAgent when a USB device is added or
   * removed.
   *
   * This can be called from either the thread that called Start() or from the
   * hotplug thread.
   */
  void HotPlugEvent(ola::usb::HotplugAgent::EventType event,
                    struct libusb_device *usb_device);

 private:
  typedef std::map<ola::usb::USBDeviceID, ola::usb::JaRuleWidget*> WidgetMap;

  ola::io::SelectServer* m_ss;
  std::auto_ptr<NotificationCallback> const m_notification_cb;
  std::auto_ptr<ola::usb::HotplugAgent> m_hotplug_agent;
  ola::thread::ExecutorThread m_cleanup_thread;
  ola::thread::ThreadId m_start_thread_id;
  bool m_in_start;
  WidgetMap m_widgets;

  void SignalEvent(EventType event, ola::usb::JaRuleWidget* widget);
  void WidgetEvent(EventType event, ola::usb::JaRuleWidget* widget,
                   ola::thread::Future<void>* f);

  DISALLOW_COPY_AND_ASSIGN(USBDeviceManager);
};

#endif  // TOOLS_JA_RULE_USBDEVICEMANAGER_H_
