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
#include <ola/thread/Future.h>
#include <ola/thread/Thread.h>
#include <ola/util/Deleter.h>

#include <memory>

#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/LibUsbThread.h"

using ola::NewCallback;
using ola::NewSingleCallback;
using ola::io::SelectServer;
using ola::thread::Future;
using ola::thread::Thread;
using ola::usb::AsynchronousLibUsbAdaptor;
using ola::usb::JaRuleWidget;
using ola::usb::LibUsbAdaptor;
using ola::usb::USBDeviceID;
using ola::usb::HotplugAgent;
using std::auto_ptr;

static const uint16_t kProductId = 0xaced;
static const uint16_t kVendorId = 0x1209;

USBDeviceManager::USBDeviceManager(SelectServer* ss,
                                   NotificationCallback* notification_cb)
    : m_ss(ss),
      m_notification_cb(notification_cb),
      m_cleanup_thread(Thread::Options("cleanup-thread")),
      m_start_thread_id(),
      m_in_start(false) {
}

USBDeviceManager::~USBDeviceManager() {
  Stop();
}

bool USBDeviceManager::Start() {
  m_start_thread_id = Thread::Self();
  m_in_start = true;

  m_hotplug_agent.reset(new HotplugAgent(
        NewCallback(this, &USBDeviceManager::HotPlugEvent), 3));

  if (!m_hotplug_agent->Init()) {
    return false;
  }

  if (!m_hotplug_agent->Start()) {
    return false;
  }

  m_cleanup_thread.Start();
  m_in_start = false;
  return true;
}

bool USBDeviceManager::Stop() {
  if (!m_hotplug_agent.get()) {
    return true;
  }

  // At this point there may be:
  //  - notifications queued on the ss.
  //  - A new event about to arrive from the HotplugAgent

  // Stop receiving notifications, this prevents any further calls to
  // HotPlugEvent.
  m_hotplug_agent->HaltNotifications();

  // Now process any callbacks on the SS so all notifications complete.
  m_ss->DrainCallbacks();

  // Clean up any remaining widgets.
  WidgetMap::iterator iter = m_widgets.begin();
  for (; iter != m_widgets.end(); ++iter) {
    if (iter->second) {
      m_notification_cb->Run(WIDGET_REMOVED, iter->second);
      m_cleanup_thread.Execute(ola::DeletePointerCallback(iter->second));
    }
  }
  m_widgets.clear();

  // Blocks until all widgets have been deleted.
  m_cleanup_thread.Stop();

  // Now we can finally stop the libusb thread.
  m_hotplug_agent->Stop();
  m_hotplug_agent.reset();
  return true;
}

void USBDeviceManager::HotPlugEvent(HotplugAgent::EventType event,
                                    struct libusb_device *usb_device) {
  // See the comments under libusb_hotplug_register_callback in the libusb
  // documentation. There are a number of caveats.
  struct libusb_device_descriptor descriptor;
  libusb_get_device_descriptor(usb_device, &descriptor);

  OLA_DEBUG << "idProduct: " << descriptor.idProduct << ", idVendor: "
            << descriptor.idVendor;

  if (descriptor.idVendor != kVendorId || descriptor.idProduct != kProductId) {
    return;
  }

  USBDeviceID device_id =
    m_hotplug_agent->GetUSBAdaptor()->GetDeviceId(usb_device);

  if (event == HotplugAgent::DEVICE_ADDED) {
    WidgetMap::iterator iter = ola::STLLookupOrInsertNull(&m_widgets,
                                                          device_id);
    if (iter->second) {
      // Dup event
      return;
    }

    auto_ptr<JaRuleWidget> widget(
        new JaRuleWidget(m_ss, m_hotplug_agent->GetUSBAdaptor(), usb_device));
    if (!widget->Init()) {
      m_widgets.erase(iter);
      return;
    }

    iter->second = widget.release();
    SignalEvent(WIDGET_ADDED, iter->second);
  } else if (event == HotplugAgent::DEVICE_REMOVED) {
    JaRuleWidget *widget = NULL;
    if (!ola::STLLookupAndRemove(&m_widgets, device_id, &widget) ||
        widget == NULL) {
      return;
    }

    SignalEvent(WIDGET_REMOVED, widget);
    /*
     * The deletion process goes something like:
     *  - cancel any pending transfers
     *  - wait for the transfer callbacks to complete (executed in the libusb
     *    thread)
     *  - close the libusb device
     *
     *  These all take place in the destructor of the JaRuleWidget.
     *
     * To avoid deadlocks, we perform the deletion in a separate thread. That
     * way ~JaRuleWidget() can block waiting for the transfer callbacks to
     * run, without having to worry about blocking the hotplug thread.
     */
    m_cleanup_thread.Execute(ola::DeletePointerCallback(widget));
  }
}

void USBDeviceManager::SignalEvent(EventType event, JaRuleWidget* widget) {
  if (!m_notification_cb.get()) {
    return;
  }

  // Because pthread_t is a struct on Windows and there's no invalid process
  // value, we separately track if we're in start or not too
  if (m_in_start && pthread_equal(m_start_thread_id, Thread::Self())) {
    // We're within Start(), so we can execute the callbacks directly.
    m_notification_cb->Run(event, widget);
  } else {
    // We're not within Start(), which means we're running on the hotplug agent
    // thread. Schedule the callback to run and wait for it to complete.
    // By waiting we ensure the callback has completed before we go ahead and
    // delete the widget.
    Future<void> f;
    m_ss->Execute(NewSingleCallback(this, &USBDeviceManager::WidgetEvent,
                                    event, widget, &f));
    f.Get();
  }
}

void USBDeviceManager::WidgetEvent(EventType event, JaRuleWidget* widget,
                                   Future<void>* f) {
  m_notification_cb->Run(event, widget);
  f->Set();
}
