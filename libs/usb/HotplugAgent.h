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
 * HotplugAgent.h
 * Handles auto-detection of USB devices.
 * Copyright (C) 2015 Simon Newton
 */

#ifndef LIBS_USB_HOTPLUGAGENT_H_
#define LIBS_USB_HOTPLUGAGENT_H_

#include <libusb.h>
#include <ola/Callback.h>
#include <ola/thread/PeriodicThread.h>

#include <map>
#include <memory>

#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/LibUsbThread.h"
#include "libs/usb/Types.h"

namespace ola {
namespace usb {

/**
 * @brief Detects when USB devices are added or removed.
 *
 * The HotplugAgent will run a callback when a USB device is added or removed.
 * On systems with libusb >= 1.0.16 which also support hotplug we'll use the
 * Hotplug API, otherwise we'll periodically check for devices.
 */
class HotplugAgent {
 public:
  enum EventType {
    DEVICE_ADDED,   //!< The device was added.
    DEVICE_REMOVED,   //!< The device ws removed.
  };

  /**
   * @brief Called when a USB device has been added or removed
   * @tparam EventType The type of the event.
   * @tparam The libusb device for this event.
   *
   * The callback can be run in either the thread calling Start() or from
   * an internal hotplug thread. However it won't be called from both at once.
   */
  typedef ola::Callback2<void, EventType, struct libusb_device*>
      NotificationCallback;

  /**
   * @brief Create a new HotplugAgent.
   * @param notification_cb The callback to run when the device is added or
   *   removed. Ownership is transferred.
   * @param debug_level The libusb debug level.
   */
  HotplugAgent(NotificationCallback* notification_cb,
               int debug_level);

  /**
   * @brief Destructor.
   *
   * This will stop the HotplugAgent if it's still running.
   */
  ~HotplugAgent();

  /**
   * @brief Get the AsynchronousLibUsbAdaptor to use.
   * @returns An AsynchronousLibUsbAdaptor, ownership is not transferred.
   * @pre Must be called after Init()
   *
   * The adaptor is valid until the call to Stop().
   */
  AsynchronousLibUsbAdaptor *GetUSBAdaptor() const;

  /**
   * @brief Initialize the hotplug agent.
   * @returns true if the agent started correctly, false otherwise.
   */
  bool Init();

  /**
   * @brief Start the hotplug agent.
   * @returns true if the agent started correctly, false otherwise.
   * @pre Init() has been called and returned true.
   */
  bool Start();

  /**
   * @brief Prevent any further notifications from occurring.
   *
   * Once this returns, the NotificationCallback will not be called.
   */
  void HaltNotifications();

  /**
   * @brief Stop the HotplugAgent.
   * @returns true if stopped correctly, false otherwise.
   *
   * Stop() may result in notifications being run, however once Stop() returns,
   * no further calls to the notification callback will be made.
   */
  bool Stop();

  #ifdef HAVE_LIBUSB_HOTPLUG_API
  /**
   * @brief Called when a USB hotplug event occurs.
   * @param dev the libusb_device the event occurred for.
   * @param event indicates if the device was added or removed.
   *
   * This can be called from either the thread that called
   * Start(), or from the libusb thread. It can't be called from both threads at
   * once though, since the libusb thread is only started once the initial call
   * to libusb_hotplug_register_callback returns.
   */
  void HotPlugEvent(struct libusb_device *dev,
                    libusb_hotplug_event event);
  #endif  // HAVE_LIBUSB_HOTPLUG_API

 private:
  typedef std::map<USBDeviceID, struct libusb_device*> DeviceMap;

  std::unique_ptr<NotificationCallback> const m_notification_cb;
  const int m_debug_level;
  bool m_use_hotplug;
  libusb_context *m_context;
  std::unique_ptr<ola::usb::LibUsbThread> m_usb_thread;
  std::unique_ptr<ola::usb::AsynchronousLibUsbAdaptor> m_usb_adaptor;
  std::unique_ptr<ola::thread::PeriodicThread> m_scanner_thread;

  ola::thread::Mutex m_mutex;
  bool m_suppress_hotplug_events;  // GUARDED_BY(m_mutex);

  // In hotplug mode, this is guarded by m_mutex while
  // m_suppress_hotplug_events is false.
  // In non-hotplug mode, this is only accessed from the scanner thread, unless
  // the thread is no longer running in which case it's accessed from the main
  // thread during cleanup
  DeviceMap m_devices;

  bool HotplugSupported();
  bool ScanUSBDevices();

  DISALLOW_COPY_AND_ASSIGN(HotplugAgent);
};
}  // namespace usb
}  // namespace ola
#endif  // LIBS_USB_HOTPLUGAGENT_H_
