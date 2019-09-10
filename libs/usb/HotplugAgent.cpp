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
 * HotplugAgent.cpp
 * Handles auto-detection of USB devices.
 * Copyright (C) 2015 Simon Newton
 */

#include "libs/usb/HotplugAgent.h"

#include <libusb.h>

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/stl/STLUtils.h>
#include <ola/thread/PeriodicThread.h>
#include <ola/thread/Mutex.h>
#include <ola/thread/Thread.h>

#include <set>
#include <utility>
#include <memory>

#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/LibUsbThread.h"

namespace ola {
namespace usb {

using ola::usb::AsynchronousLibUsbAdaptor;
using ola::usb::LibUsbAdaptor;
using std::auto_ptr;
using std::pair;

namespace {
#ifdef HAVE_LIBUSB_HOTPLUG_API
int LIBUSB_CALL hotplug_callback(OLA_UNUSED struct libusb_context *ctx,
                                 struct libusb_device *dev,
                                 libusb_hotplug_event event,
                                 void *user_data) {
  HotplugAgent *agent = reinterpret_cast<HotplugAgent*>(user_data);
  agent->HotPlugEvent(dev, event);
  return 0;
}
#endif  // HAVE_LIBUSB_HOTPLUG_API
}  // namespace

HotplugAgent::HotplugAgent(NotificationCallback* notification_cb,
                           int debug_level)
    : m_notification_cb(notification_cb),
      m_debug_level(debug_level),
      m_use_hotplug(false),
      m_context(NULL),
      m_suppress_hotplug_events(false) {
}

HotplugAgent::~HotplugAgent() {
  if (m_context) {
    Stop();
  }
}

AsynchronousLibUsbAdaptor *HotplugAgent::GetUSBAdaptor() const {
  return m_usb_adaptor.get();
}

bool HotplugAgent::Init() {
  if (!LibUsbAdaptor::Initialize(&m_context)) {
    return false;
  }

#ifdef HAVE_LIBUSB_SET_OPTION
  OLA_DEBUG << "libusb_set_option(LIBUSB_OPTION_LOG_LEVEL, " << m_debug_level
            << ")";
  libusb_set_option(m_context, LIBUSB_OPTION_LOG_LEVEL, m_debug_level);
#else
  OLA_DEBUG << "libusb_set_debug(" << m_debug_level << ")";
  libusb_set_debug(m_context, m_debug_level);
#endif  // HAVE_LIBUSB_SET_OPTION

  m_use_hotplug = ola::usb::LibUsbAdaptor::HotplugSupported();
  OLA_DEBUG << "HotplugSupported(): " << m_use_hotplug;
#ifdef HAVE_LIBUSB_HOTPLUG_API
  if (m_use_hotplug) {
    m_usb_thread.reset(new ola::usb::LibUsbHotplugThread(
          m_context, hotplug_callback, this));
  }
#endif  // HAVE_LIBUSB_HOTPLUG_API

  if (!m_usb_thread.get()) {
    m_usb_thread.reset(new ola::usb::LibUsbSimpleThread(m_context));
  }
  m_usb_adaptor.reset(
      new ola::usb::AsynchronousLibUsbAdaptor(m_usb_thread.get()));
  return true;
}

bool HotplugAgent::Start() {
  // If we're using hotplug, this starts the hotplug thread.
  if (!m_usb_thread->Init()) {
    m_usb_adaptor.reset();
    m_usb_thread.reset();
    return false;
  }

  if (!m_use_hotplug) {
    // Either we don't support hotplug or the setup failed.
    // As poor man's hotplug, we call libusb_get_device_list periodically to
    // check for new devices.
    m_scanner_thread.reset(new ola::thread::PeriodicThread(
          TimeInterval(5, 0),
          NewCallback(this, &HotplugAgent::ScanUSBDevices)));
  }
  return true;
}

void HotplugAgent::HaltNotifications() {
  // To prevent any further notifications, we need to either:
  //  - suppress hotplug events so we don't add any new devices.
  //    OR
  //  - Stop the scanner thread, same idea above applies.
  if (m_scanner_thread.get()) {
    m_scanner_thread->Stop();
  }

  {
    ola::thread::MutexLocker locker(&m_mutex);
    m_suppress_hotplug_events = true;
  }
}

bool HotplugAgent::Stop() {
  // Prevent any further notifications if we haven't already.
  // Once this completes, we're free to access m_devices without a lock.
  HaltNotifications();

  m_devices.clear();

  // Stop the usb_thread (if using hotplug, otherwise this is a noop).
  m_usb_thread->Shutdown();

  m_usb_thread.reset();
  m_usb_adaptor.reset();
  libusb_exit(m_context);
  m_context = NULL;
  return true;
}


#ifdef HAVE_LIBUSB_HOTPLUG_API
void HotplugAgent::HotPlugEvent(struct libusb_device *usb_device,
                                libusb_hotplug_event event) {
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_suppress_hotplug_events) {
    return;
  }

  USBDeviceID device_id = m_usb_adaptor->GetDeviceId(usb_device);

  OLA_INFO << "USB hotplug event: " << device_id << " @" << usb_device << " ["
           << (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED ? "add" : "del")
           << "]";

  if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
    pair<DeviceMap::iterator, bool> p = m_devices.insert(
        DeviceMap::value_type(device_id, usb_device));

    if (!p.second) {
      // already an entry in the map
      if (p.first->second != usb_device) {
        OLA_WARN << "Received double hotplug notification for "
                 << device_id;
      }
      return;
    }

    m_notification_cb->Run(DEVICE_ADDED, usb_device);

  } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
    DeviceMap::iterator iter = m_devices.find(device_id);
    if (iter == m_devices.end()) {
      OLA_WARN << "Failed to find " << device_id;
      return;
    }

    if (iter->second != usb_device) {
      OLA_WARN << "Device mismatch for " << device_id;
      return;
    }

    m_devices.erase(iter);
    m_notification_cb->Run(DEVICE_REMOVED, usb_device);
  }
}
#endif  // HAVE_LIBUSB_HOTPLUG_API

/**
 * @brief Check if this platform supports hotplug.
 * @returns true if hotplug is supported and enabled on this platform, false
 *   otherwise.
 * @deprecated This is only here for backwards compatibility. New code should
 *   use ola::usb::LibUsbAdaptor::HotplugSupported().
 */
bool HotplugAgent::HotplugSupported() {
  return ola::usb::LibUsbAdaptor::HotplugSupported();
}

/*
 * If hotplug isn't supported, this is called periodically to check for
 * USB devices that have been added or removed.
 */
bool HotplugAgent::ScanUSBDevices() {
  std::set<USBDeviceID> current_device_ids;

  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(m_context, &device_list);

  for (unsigned int i = 0; i < device_count; i++) {
    libusb_device *usb_device = device_list[i];

    USBDeviceID device_id = m_usb_adaptor->GetDeviceId(usb_device);
    current_device_ids.insert(device_id);

    pair<DeviceMap::iterator, bool> p = m_devices.insert(
        DeviceMap::value_type(device_id, usb_device));
    if (p.second) {
      m_notification_cb->Run(DEVICE_ADDED, usb_device);
    }
  }
  libusb_free_device_list(device_list, 1);  // unref devices

  // Remove any old ones.
  DeviceMap::iterator iter = m_devices.begin();
  while (iter != m_devices.end()) {
    if (!STLContains(current_device_ids, iter->first)) {
      m_notification_cb->Run(DEVICE_REMOVED, iter->second);
      m_devices.erase(iter++);
    } else {
      iter++;
    }
  }
  return true;
}
}  // namespace usb
}  // namespace ola
