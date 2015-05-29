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
 * USBDeviceManager.cpp
 * Handles auto-detection of Ja Rule widgets.
 * Copyright (C) 2015 Simon Newton
 */

#include "tools/ja-rule/USBDeviceManager.h"

#include <libusb.h>

#include <ola/Callback.h>
#include <ola/Logging.h>
#include <ola/io/SelectServer.h>
#include <ola/stl/STLUtils.h>
#include <ola/thread/ExecutorThread.h>
#include <ola/thread/Future.h>
#include <ola/thread/Mutex.h>
#include <ola/thread/Thread.h>
#include <ola/util/Deleter.h>

#include <memory>

#include "plugins/usbdmx/LibUsbAdaptor.h"
#include "plugins/usbdmx/LibUsbThread.h"

using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::plugin::usbdmx::LibUsbAdaptor;
using ola::plugin::usbdmx::LibUsbHotplugThread;
using ola::thread::Future;
using ola::thread::MutexLocker;
using ola::thread::Thread;
using std::auto_ptr;

static const uint16_t kProductId = 0x0053;
static const uint16_t kVendorId = 0x04d8;

namespace {

int hotplug_callback(OLA_UNUSED struct libusb_context *ctx,
                     struct libusb_device *dev,
                     libusb_hotplug_event event,
                     void *user_data) {
  USBDeviceManager *manager = reinterpret_cast<USBDeviceManager*>(user_data);
  manager->HotPlugEvent(dev, event);
  return 0;
}
}  // namespace

USBDeviceManager::USBDeviceManager(SelectServer* ss,
                                   NotificationCallback* notification_cb)
    : m_ss(ss),
      m_cleanup_thread(Thread::Options("cleanup-thread")),
      m_context(NULL),
      m_start_thread_id(0),
      m_notification_cb(notification_cb),
      m_suppress_hotplug_events(false) {
}

USBDeviceManager::~USBDeviceManager() {
  if (m_context) {
    Stop();
  }
}

bool USBDeviceManager::Start() {
  int r = libusb_init(&m_context);

  if (r < 0) {
    OLA_WARN << "libusb_init() failed: " << LibUsbAdaptor::ErrorCodeToString(r);
    m_cleanup_thread.Stop();
    return false;
  }

  libusb_set_debug(m_context, 3);

  m_start_thread_id = Thread::Self();
  m_usb_thread.reset(
      new LibUsbHotplugThread(m_context, hotplug_callback, this));
  bool ok = m_usb_thread->Init();
  if (ok) {
    m_cleanup_thread.Start();
  } else {
    m_usb_thread.reset();
    libusb_exit(m_context);
    m_context = NULL;
  }

  return ok;
}

bool USBDeviceManager::Stop() {
  // At this point there may be:
  //  - notifications queued on the ss.
  //  - IP in the critical section of HotPlugEvent
  //  - IP prior to the critical section of HotPlugEvent
  {
    MutexLocker locker(&m_mutex);
    m_suppress_hotplug_events = true;
  }

  // Now we're left with:
  //  - Notifications queued on the ss.
  //  - IP prior to the critical section of HotPlugEvent, these are now
  //    surpressed.
  // Drain callbacks on the SS so all notifications complete.
  m_ss->DrainCallbacks();

  {
    MutexLocker locker(&m_mutex);
    DeviceMap::iterator iter = m_devices.begin();
    for (; iter != m_devices.end(); ++iter) {
      if (iter->second) {
        m_notification_cb->Run(DEVICE_REMOVED, iter->second);
        DeleteDevice(iter->second);
      }
    }
    m_devices.clear();
  }

  // Blocks until all devices have been deleted.
  m_cleanup_thread.Stop();

  m_usb_thread->Shutdown();
  m_usb_thread.reset();
  libusb_exit(m_context);
  m_context = NULL;
  return true;
}

void USBDeviceManager::HotPlugEvent(struct libusb_device *usb_device,
                                    libusb_hotplug_event event) {
  // See the comments under libusb_hotplug_register_callback in the libusb
  // documentation. There are a number of caveats.
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(usb_device, &descriptor);

  OLA_DEBUG << "idProduct: " << descriptor.idProduct << ", idVendor: "
            << descriptor.idVendor;

  if (descriptor.idVendor != kVendorId || descriptor.idProduct != kProductId) {
    return;
  }

  USBDeviceID device_id(libusb_get_bus_number(usb_device),
                        libusb_get_device_address(usb_device));

  if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
    DeviceAdded(usb_device, device_id);
  } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
    DeviceRemoved(device_id);
  }
}

void USBDeviceManager::DeviceAdded(struct libusb_device* usb_device,
                                   const USBDeviceID& device_id) {
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_suppress_hotplug_events) {
    // Stop() has been called.
    return;
  }

  DeviceMap::iterator iter = ola::STLLookupOrInsertNull(&m_devices,
                                                        device_id);
  if (iter->second) {
    // Dup event
    return;
  }

  OLA_INFO << "Open Lighting Device connected";
  auto_ptr<JaRuleEndpoint> device(new JaRuleEndpoint(m_ss, usb_device));
  if (!device->Init()) {
    m_devices.erase(iter);
    return;
  }

  iter->second = device.release();

  // We've finished manipulating m_devices, but we need to ensure the callback
  // is queued before releasing the lock.
  SignalEvent(DEVICE_ADDED, iter->second, &locker);
}

void USBDeviceManager::DeviceRemoved(const USBDeviceID& device_id) {
  ola::thread::MutexLocker locker(&m_mutex);
  if (m_suppress_hotplug_events) {
    // Stop() has been called.
    return;
  }

  JaRuleEndpoint *device = NULL;
  if (!ola::STLLookupAndRemove(&m_devices, device_id, &device) ||
      device == NULL) {
    return;
  }

  OLA_INFO << "Open Lighting Device disconnected";
  SignalEvent(DEVICE_REMOVED, device, &locker);
  DeleteDevice(device);
}

void USBDeviceManager::SignalEvent(EventType event,
                                   JaRuleEndpoint* device,
                                   MutexLocker* locker) {
  // We hold the lock at this point.
  if (!m_notification_cb.get()) {
    return;
  }

  if (pthread_equal(m_start_thread_id, Thread::Self())) {
    locker->Release();
    // We're within Start(), so we can execute the callbacks directly.
    m_notification_cb->Run(event, device);
  } else {
    // We're not within Start(), which means we're running on the libusb event
    // thread. Schedule the callback to run and wait for it to complete.
    // By waiting we ensure the callback has completed before we go ahead and
    // delete the device.
    Future<void> f;
    m_ss->Execute(NewSingleCallback(this, &USBDeviceManager::DeviceEvent,
                                    event, device, &f));
    locker->Release();
    f.Get();
  }
}

void USBDeviceManager::DeviceEvent(EventType event, JaRuleEndpoint* device,
                                   Future<void>* f) {
  m_notification_cb->Run(event, device);
  f->Set();
}

/*
 * @brief Schedule deletion of the device.
 *
 * The deletion process goes something like:
 *  - cancel any pending transfers
 *  - wait for the transfer callbacks to complete (executed in the libusb
 *    thread)
 *  - close the libusb device
 *
 *  These all take place in the destructor of the JaRuleEndpoint.
 *
 * To avoid deadlocks, we perform the deletion in a separate thread. That
 * way ~JaRuleEndpoint() can block waiting for the transfer callbacks to
 * run, without having to worry about blocking the main thread.
 */
void USBDeviceManager::DeleteDevice(JaRuleEndpoint* device) {
  m_cleanup_thread.Execute(ola::DeletePointerCallback(device));
}

