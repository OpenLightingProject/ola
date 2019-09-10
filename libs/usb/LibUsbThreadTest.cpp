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
 * LibUsbThreadTest.cpp
 * Test fixture for the LibUsbThread class
 * Copyright (C) 2014 Simon Newton
 */

#include <libusb.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libs/usb/LibUsbAdaptor.h"
#include "libs/usb/LibUsbThread.h"
#include "ola/Logging.h"
#include "ola/testing/TestUtils.h"

namespace {
#if HAVE_LIBUSB_HOTPLUG_API
int LIBUSB_CALL hotplug_callback(OLA_UNUSED struct libusb_context *ctx,
                                 OLA_UNUSED struct libusb_device *dev,
                                 OLA_UNUSED libusb_hotplug_event event,
                                 OLA_UNUSED void *user_data) {
  return 0;
}
#endif  // HAVE_LIBUSB_HOTPLUG_API
}  // namespace

class LibUsbThreadTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(LibUsbThreadTest);
  CPPUNIT_TEST(testNonHotplug);
#if HAVE_LIBUSB_HOTPLUG_API
  CPPUNIT_TEST(testHotplug);
#endif  // HAVE_LIBUSB_HOTPLUG_API
  CPPUNIT_TEST_SUITE_END();

 public:
  LibUsbThreadTest() : m_context(NULL) {}

  void setUp();
  void tearDown();

  void testNonHotplug();
#if HAVE_LIBUSB_HOTPLUG_API
  void testHotplug();
#endif  // HAVE_LIBUSB_HOTPLUG_API

 private:
  libusb_context *m_context;

  void AttemptDeviceOpen(ola::usb::LibUsbThread *thread);
};

CPPUNIT_TEST_SUITE_REGISTRATION(LibUsbThreadTest);

void LibUsbThreadTest::setUp() {
  if (libusb_init(&m_context)) {
    OLA_INFO << "Failed to init libusb";
  }
}

void LibUsbThreadTest::tearDown() {
  if (m_context) {
    libusb_exit(m_context);
  }
}

void LibUsbThreadTest::testNonHotplug() {
  if (!m_context) {
    return;
  }

  ola::usb::LibUsbSimpleThread thread(m_context);
  OLA_ASSERT_TRUE(thread.Init());
  AttemptDeviceOpen(&thread);
}

#if HAVE_LIBUSB_HOTPLUG_API
void LibUsbThreadTest::testHotplug() {
  if (!m_context) {
    return;
  }

  bool hotplug_support = ola::usb::LibUsbAdaptor::HotplugSupported();
  OLA_DEBUG << "HotplugSupported(): " << hotplug_support;
  ola::usb::LibUsbHotplugThread thread(m_context, hotplug_callback, NULL);
  if (hotplug_support) {
    OLA_ASSERT_TRUE(thread.Init());
    AttemptDeviceOpen(&thread);
    thread.Shutdown();
  } else {
    OLA_WARN << "No hotplug support, check that starting the thread fails";
    OLA_ASSERT_FALSE(thread.Init());
  }
}
#endif  // HAVE_LIBUSB_HOTPLUG_API

/*
 * Try to open any USB device so we can test interaction with the thread.
 */
void LibUsbThreadTest::AttemptDeviceOpen(ola::usb::LibUsbThread *thread) {
  libusb_device_handle *usb_handle = NULL;
  libusb_device **device_list;
  size_t device_count = libusb_get_device_list(m_context, &device_list);

  for (unsigned int i = 0; i < device_count; i++) {
    libusb_device *usb_device = device_list[i];

    if (libusb_open(usb_device, &usb_handle) == 0) {
      thread->OpenHandle();
      break;
    }
  }
  if (usb_handle) {
    thread->CloseHandle(usb_handle);
  }
  libusb_free_device_list(device_list, 1);
}
