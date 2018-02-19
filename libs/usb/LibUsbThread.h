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
 * LibUsbThread.h
 * The thread for asynchronous libusb communication.
 * Copyright (C) 2014 Simon Newton
 */

#ifndef LIBS_USB_LIBUSBTHREAD_H_
#define LIBS_USB_LIBUSBTHREAD_H_

#include <libusb.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include "ola/base/Macro.h"
#include "ola/thread/Thread.h"

namespace ola {
namespace usb {

/**
 * @brief The base class for the dedicated libusb thread.
 *
 * Asynchronous I/O for libusb requires either i) a dedicated thread ii)
 * integration with the i/o event loop. From the libusb documentation, i) has
 * the advantage that it works on Windows, so we do that.
 *
 * However, there is no easy way to interrupt libusb_handle_events(). Instead
 * we use either libusb_close (for the non-hotplug case) or
 * libusb_hotplug_deregister_callback() (for the hotplug case) to wake
 * libusb_handle_events().
 *
 * Both these techniques have require care to avoid deadlocks / race
 * conditions. For the non-hotplug case, it's imperative that libusb_open() and
 * libusb_close() are paired with calls to OpenHandle() and CloseHandle().
 *
 * http://libusb.sourceforge.net/api-1.0/group__asyncio.html covers both
 * approaches.
 */
class LibUsbThread : private ola::thread::Thread {
 public:
  /**
   * @brief Base constructor
   * @param context the libusb context to use.
   */
  explicit LibUsbThread(libusb_context *context)
    : m_context(context),
      m_term(false) {
  }

  /**
   * @brief Destructor.
   */
  virtual ~LibUsbThread() {}

  /**
   * @brief Initialize the thread.
   */
  virtual bool Init() {
    return true;
  }

  /**
   * @brief Shutdown the thread.
   */
  virtual void Shutdown() {}

  /**
   * @brief The entry point to the libusb thread.
   *
   * Don't call this directly. It's executed when the thread starts.
   */
  void *Run();

  /**
   * @brief This must be called whenever libusb_open() is called.
   */
  virtual void OpenHandle() = 0;

  /**
   * @brief This must be called whenever libusb_close() is called.
   */
  virtual void CloseHandle(libusb_device_handle *handle) = 0;

 protected:
  /**
   * @brief Indicate that the libusb thread should terminate.
   *
   * This doesn't wake up libusb_handle_events(), it simply sets m_term to
   * true.
   */
  void SetTerminate() {
    ola::thread::MutexLocker locker(&m_term_mutex);
    m_term = true;
  }

  /**
   * @brief Start the libusb thread.
   */
  void LaunchThread();

  /**
   * @brief Join the libusb thread.
   */
  void JoinThread();

  /**
   * @brief Return the libusb_context this thread uses.
   * @returns A libusb_context.
   */
  libusb_context* Context() const { return m_context; }

 private:
  libusb_context *m_context;
  bool m_term;  // GUARDED_BY(m_term_mutex)
  ola::thread::Mutex m_term_mutex;
};

#if HAVE_LIBUSB_HOTPLUG_API

/**
 * @brief The hotplug version of the LibUsbThread.
 */
class LibUsbHotplugThread : public LibUsbThread {
 public:
  /**
   * @brief Create a new LibUsbHotplugThread
   * @param context the libusb context to use.
   * @param callback_fn The callback function to run when hotplug events occur.
   * @param user_data User data to pass to the callback function.
   *
   * The thread is started in Init(). When the object is
   * destroyed, the handle is de-registered as part of the thread shutdown
   * sequence.
   */
  LibUsbHotplugThread(libusb_context *context,
                      libusb_hotplug_callback_fn callback_fn,
                      void *user_data);

  bool Init();

  void Shutdown();

  void OpenHandle() {}

  void CloseHandle(libusb_device_handle *handle);

 private:
  libusb_hotplug_callback_handle m_hotplug_handle;
  libusb_hotplug_callback_fn m_callback_fn;
  void *m_user_data;

  DISALLOW_COPY_AND_ASSIGN(LibUsbHotplugThread);
};

#endif  // HAVE_LIBUSB_HOTPLUG_API

/**
 * @brief The non-hotplug version of LibUsbThread.
 *
 * The libusb thread is only run when one of more handles are open. Otherwise
 * there is no way to interrupt libusb_handle_events(). See the libusb Async
 * documentation at http://libusb.sourceforge.net/api-1.0/group__asyncio.html
 * for more information.
 */
class LibUsbSimpleThread : public LibUsbThread {
 public:
  /**
   * @brief Create a new LibUsbHotplugThread
   * @param context the libusb context to use.
   *
   * The thread is starts as soon as this object is created. When the object is
   * destroyed, the handle is de-registered as part of the thread shutdown
   * sequence.
   */
  explicit LibUsbSimpleThread(libusb_context *context)
    : LibUsbThread(context),
      m_device_count(0) {
  }

  void OpenHandle();
  void CloseHandle(libusb_device_handle *handle);

 private:
  unsigned int m_device_count;

  DISALLOW_COPY_AND_ASSIGN(LibUsbSimpleThread);
};
}  // namespace usb
}  // namespace ola
#endif  // LIBS_USB_LIBUSBTHREAD_H_
