/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * UsbProWidgetDetectorTest.cpp
 * Test fixture for the UsbProWidgetDetector class
 * Copyright (C) 2010 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>

#include "ola/testing/TestUtils.h"

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/CommonWidgetTest.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"


using ola::io::ConnectedDescriptor;
using ola::plugin::usbpro::UsbProWidgetInformation;
using ola::plugin::usbpro::UsbProWidgetDetector;
using std::string;


class UsbProWidgetDetectorTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(UsbProWidgetDetectorTest);
  CPPUNIT_TEST(testExtendedDiscovery);
  CPPUNIT_TEST(testDiscovery);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST(testSniffer);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testExtendedDiscovery();
    void testDiscovery();
    void testTimeout();
    void testSniffer();

  private:
    auto_ptr<UsbProWidgetDetector> m_detector;
    UsbProWidgetInformation m_device_info;
    bool m_found_widget;
    bool m_failed_widget;

    void NewWidget(ConnectedDescriptor *descriptor,
                   const UsbProWidgetInformation *info);
    void FailedWidget(ConnectedDescriptor *descriptor);
    void Timeout() { m_ss.Terminate(); }

    static const uint8_t DEVICE_LABEL = 78;
    static const uint8_t MANUFACTURER_LABEL = 77;
    static const uint8_t SERIAL_LABEL = 10;
    static const uint8_t SNIFFER_LABEL = 0x81;
};


CPPUNIT_TEST_SUITE_REGISTRATION(UsbProWidgetDetectorTest);


void UsbProWidgetDetectorTest::setUp() {
  CommonWidgetTest::setUp();
  m_found_widget = false;
  m_failed_widget = false;

  m_detector.reset(new UsbProWidgetDetector(
      &m_ss,
      ola::NewCallback(this, &UsbProWidgetDetectorTest::NewWidget),
      ola::NewCallback(this, &UsbProWidgetDetectorTest::FailedWidget),
      10));

  m_ss.RegisterSingleTimeout(
      4000,  // This should only take 40ms, but on slow platforms takes longer
      ola::NewSingleCallback(this, &UsbProWidgetDetectorTest::Timeout));
}


void UsbProWidgetDetectorTest::NewWidget(ConnectedDescriptor *descriptor,
                                         const UsbProWidgetInformation *info) {
  OLA_ASSERT_EQ(
      static_cast<ConnectedDescriptor*>(&m_descriptor),
      descriptor);
  m_found_widget = true;
  m_device_info = *info;
  m_ss.Terminate();
  delete info;
}


void UsbProWidgetDetectorTest::FailedWidget(ConnectedDescriptor *descriptor) {
  OLA_ASSERT_EQ(
      static_cast<ConnectedDescriptor*>(&m_descriptor),
      descriptor);
  m_failed_widget = true;
  m_ss.Terminate();
}


/**
 * Test that discovery works for a device that implements the extended set of
 * messages.
 */
void UsbProWidgetDetectorTest::testExtendedDiscovery() {
  uint32_t expected_serial = 0x12345678;
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  uint16_t expected_manufacturer = 0x7a70;
  uint8_t manufacturer_data[] = "pzOpen Lighting";
  uint16_t expected_device = 0x534e;
  uint8_t device_data[] = "NSUnittest Device";
  m_endpoint->AddExpectedUsbProDataAndReturn(
      MANUFACTURER_LABEL,
      NULL,
      0,
      MANUFACTURER_LABEL,
      manufacturer_data,
      sizeof(manufacturer_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      DEVICE_LABEL,
      NULL,
      0,
      DEVICE_LABEL,
      device_data,
      sizeof(device_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      SERIAL_LABEL,
      NULL,
      0,
      SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_detector->Discover(&m_descriptor);
  m_ss.Run();

  OLA_ASSERT(m_found_widget);
  OLA_ASSERT_FALSE(m_failed_widget);

  OLA_ASSERT_EQ(expected_manufacturer, m_device_info.esta_id);
  OLA_ASSERT_EQ(expected_device, m_device_info.device_id);
  OLA_ASSERT_EQ(string("Open Lighting"), m_device_info.manufacturer);
  OLA_ASSERT_EQ(string("Unittest Device"), m_device_info.device);
  OLA_ASSERT_EQ(expected_serial, m_device_info.serial);
  OLA_ASSERT_EQ(expected_serial, m_device_info.serial);
}


/*
 * Check that discovery works for a device that just implements the serial #
 */
void UsbProWidgetDetectorTest::testDiscovery() {
  uint32_t expected_serial = 0x12345678;
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  m_endpoint->AddExpectedUsbProMessage(MANUFACTURER_LABEL, NULL, 0);
  m_endpoint->AddExpectedUsbProMessage(DEVICE_LABEL, NULL, 0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      SERIAL_LABEL,
      NULL,
      0,
      SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_detector->Discover(&m_descriptor);
  m_ss.Run();

  OLA_ASSERT(m_found_widget);
  OLA_ASSERT_FALSE(m_failed_widget);

  OLA_ASSERT_EQ(static_cast<uint16_t>(0), m_device_info.esta_id);
  OLA_ASSERT_EQ(static_cast<uint16_t>(0), m_device_info.device_id);
  OLA_ASSERT_EQ(string(), m_device_info.manufacturer);
  OLA_ASSERT_EQ(string(), m_device_info.device);
  OLA_ASSERT_EQ(expected_serial, m_device_info.serial);
}


/**
 * Check a widget that fails to respond
 */
void UsbProWidgetDetectorTest::testTimeout() {
  m_endpoint->AddExpectedUsbProMessage(MANUFACTURER_LABEL, NULL, 0);
  m_endpoint->AddExpectedUsbProMessage(DEVICE_LABEL, NULL, 0);
  m_endpoint->AddExpectedUsbProMessage(SERIAL_LABEL, NULL, 0);
  m_detector->Discover(&m_descriptor);

  m_ss.Run();
  OLA_ASSERT_FALSE(m_found_widget);
  OLA_ASSERT(m_failed_widget);
}

/*
 * Check that we recognize Enttec sniffers.
 */
void UsbProWidgetDetectorTest::testSniffer() {
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  m_endpoint->AddExpectedUsbProMessage(MANUFACTURER_LABEL, NULL, 0);
  m_endpoint->AddExpectedUsbProMessage(DEVICE_LABEL, NULL, 0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      SERIAL_LABEL,
      NULL,
      0,
      SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_endpoint->SendUnsolicitedUsbProData(SNIFFER_LABEL, NULL, 0);
  m_endpoint->SendUnsolicitedUsbProData(SNIFFER_LABEL, NULL, 0);

  m_detector->Discover(&m_descriptor);
  m_ss.Run();

  OLA_ASSERT_FALSE(m_found_widget);
  OLA_ASSERT(m_failed_widget);
}

