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
#include <ola/thread/Mutex.h>
#include <ola/thread/Thread.h>

#include <map>
#include <memory>
#include <utility>

#include "plugins/usbdmx/LibUsbThread.h"
#include "tools/ja-rule/JaRuleEndpoint.h"

/**
 * @brief Manages adding / removing Open Lighting Devices.
 *
 * As Open Lighting USB devices are added or removed, this executes the
 * callback to notify the recipient.
 *
 * Currently only a single device is supported.
 */
class USBDeviceManager {
 public:
  enum EventType {
    DEVICE_ADDED,   //!< The device was added.
    DEVICE_REMOVED,   //!< The device ws removed.
  };

  /**
   * @brief Indicates a device has been added or removed
   */
  typedef ola::Callback2<void, EventType, JaRuleEndpoint*>
      NotificationCallback;

  /**
   * @brief Create a new USBDeviceManager.
   * @param ss The executor to run the notification_cb on.
   * @param notification_cb The callback to run when the device is added or
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
   * @brief Called by the hotplug notification when a USB device is added or
   * removed.
   *
   * Due to the way libusb works, this can be called from either the thread
   * that called Start() or from the libusb thread.
   */
  void HotPlugEvent(struct libusb_device *usb_device,
                    libusb_hotplug_event event);

 private:
  typedef std::pair<uint8_t, uint8_t> USBDeviceID;
  typedef std::map<USBDeviceID, JaRuleEndpoint*> DeviceMap;

  ola::io::SelectServer* m_ss;
  ola::thread::ExecutorThread m_cleanup_thread;

  libusb_context* m_context;
  std::auto_ptr<ola::plugin::usbdmx::LibUsbHotplugThread> m_usb_thread;
  ola::thread::ThreadId m_start_thread_id;

  std::auto_ptr<NotificationCallback> const m_notification_cb;

  ola::thread::Mutex m_mutex;
  bool m_suppress_hotplug_events;  // GUARDED_BY(m_mutex);
  DeviceMap m_devices;  // GUARDED_BY(m_mutex)

  void DeviceAdded(struct libusb_device* usb_device,
                   const USBDeviceID& device_id);
  void DeviceRemoved(const USBDeviceID& device_id);

  void SignalEvent(EventType event, JaRuleEndpoint* device,
                   ola::thread::MutexLocker* locker);
  void DeviceEvent(EventType event, JaRuleEndpoint* device,
                   ola::thread::Future<void>* f);

  void DeleteDevice(JaRuleEndpoint* device);

  DISALLOW_COPY_AND_ASSIGN(USBDeviceManager);
};

#endif  // TOOLS_JA_RULE_USBDEVICEMANAGER_H_
