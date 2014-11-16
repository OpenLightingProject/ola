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
 * AsyncPluginImpl.cpp
 * The asynchronous libusb implementation.
 * Copyright (C) 2014 Simon Newton
 */

#include "plugins/usbdmx/LibUsbThread.h"

#include "ola/Logging.h"
#include "ola/StringUtils.h"
#include "ola/stl/STLUtils.h"


namespace ola {
namespace plugin {
namespace usbdmx {

// LibUsbThread
// -----------------------------------------------------------------------------

void *LibUsbThread::Run() {
  OLA_INFO << "----libusb event thread is running";
  while (1) {
    {
      ola::thread::MutexLocker locker(&m_term_mutex);
      if (m_term)
        break;
    }
    libusb_handle_events(m_context);
  }
  OLA_INFO << "----libusb thread exiting";
  return NULL;
}

void LibUsbThread::LaunchThread() {
  Start();
}

void LibUsbThread::JoinThread() {
  Join();
  m_term = false;
}

// LibUsbHotplugThread
// -----------------------------------------------------------------------------

LibUsbHotplugThread::LibUsbHotplugThread(libusb_context *context,
                                         libusb_hotplug_callback_fn callback_fn,
                                         void *user_data)
    : LibUsbThread(context),
      m_hotplug_handle(0),
      m_callback_fn(callback_fn),
      m_user_data(user_data) {
  OLA_INFO << "-- Starting libusb thread";
}

LibUsbHotplugThread::~LibUsbHotplugThread() {
}

bool LibUsbHotplugThread::Init() {
  int rc = libusb_hotplug_register_callback(
      NULL,
      static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                        LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
      LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY,
      LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
      m_callback_fn, m_user_data, &m_hotplug_handle);

  if (LIBUSB_SUCCESS != rc) {
    OLA_WARN << "Error creating a hotplug callback";
    return false;
  }
  OLA_INFO << "libusb_hotplug_register_callback passed";
  LaunchThread();
  return true;
}

void LibUsbHotplugThread::Shutdown() {
  OLA_INFO << "-- Stopping libusb thread";
  SetTerminate();
  libusb_hotplug_deregister_callback(Context(), m_hotplug_handle);
  JoinThread();
}

void LibUsbHotplugThread::CloseHandle(libusb_device_handle *handle) {
  libusb_close(handle);
}

// LibUsbSimpleThread
// -----------------------------------------------------------------------------

void LibUsbSimpleThread::OpenHandle() {
  m_device_count++;
  if (m_device_count == 1) {
    OLA_INFO << "-- Starting libusb thread";
    LaunchThread();
  }
}

void LibUsbSimpleThread::CloseHandle(libusb_device_handle *handle) {
  OLA_INFO << "LibUsbSimpleThread::CloseHandle, count is " << m_device_count;
  if (m_device_count == 1) {
    SetTerminate();
  }
  libusb_close(handle);
  if (m_device_count == 1) {
    OLA_INFO << "-- Stopping libusb thread";
    JoinThread();
  }
  m_device_count--;
  OLA_INFO << "exit LibUsbSimpleThread::CloseHandle, count is "
           << m_device_count;
}
}  // namespace usbdmx
}  // namespace plugin
}  // namespace ola
