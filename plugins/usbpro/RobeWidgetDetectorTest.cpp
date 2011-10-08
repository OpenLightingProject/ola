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
 * RobeWidgetDetectorTest.cpp
 * Test fixture for the RobeWidgetDetector class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/CommonWidgetTest.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"


using ola::network::ConnectedDescriptor;
using ola::plugin::usbpro::RobeWidget;
using ola::plugin::usbpro::RobeWidgetDetector;
using ola::plugin::usbpro::RobeWidgetInformation;
using ola::rdm::UID;


class RobeWidgetDetectorTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(RobeWidgetDetectorTest);
  CPPUNIT_TEST(testRUIDevice);
  CPPUNIT_TEST(testLockedRUIDevice);
  CPPUNIT_TEST(testWTXDevice);
  CPPUNIT_TEST(testUnknownDevice);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testRUIDevice();
    void testLockedRUIDevice();
    void testWTXDevice();
    void testUnknownDevice();
    void testTimeout();

  private:
    auto_ptr<RobeWidgetDetector> m_detector;
    RobeWidgetInformation m_device_info;
    bool m_found_widget;
    bool m_failed_widget;

    void NewWidget(RobeWidget *widget,
                   const RobeWidgetInformation *info);
    void FailedWidget(ConnectedDescriptor *descriptor);
    void Timeout() { m_ss.Terminate(); }

    static const uint8_t INFO_REQUEST_LABEL = 0x14;
    static const uint8_t INFO_RESPONSE_LABEL = 0x15;
    static const uint8_t UID_REQUEST_LABEL = 0x24;
    static const uint8_t UID_RESPONSE_LABEL = 0x25;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RobeWidgetDetectorTest);


void RobeWidgetDetectorTest::setUp() {
  CommonWidgetTest::setUp();
  m_found_widget = false;
  m_failed_widget = false;

  m_detector.reset(new RobeWidgetDetector(
      &m_ss,
      ola::NewCallback(this, &RobeWidgetDetectorTest::NewWidget),
      ola::NewCallback(this, &RobeWidgetDetectorTest::FailedWidget),
      10));

  m_ss.RegisterSingleTimeout(
      40,  // 40ms should be enough
      ola::NewSingleCallback(this, &RobeWidgetDetectorTest::Timeout));
}


void RobeWidgetDetectorTest::NewWidget(RobeWidget *widget,
                                       const RobeWidgetInformation *info) {
  CPPUNIT_ASSERT_EQUAL(
      static_cast<ConnectedDescriptor*>(&m_descriptor),
      widget->GetDescriptor());
  m_found_widget = true;
  m_device_info = *info;
  m_ss.Terminate();
  delete widget;
  delete info;
}


void RobeWidgetDetectorTest::FailedWidget(ConnectedDescriptor *descriptor) {
  CPPUNIT_ASSERT_EQUAL(
      static_cast<ConnectedDescriptor*>(&m_descriptor),
      descriptor);
  m_failed_widget = true;
  m_ss.Terminate();
}


/*
 * Check that discovery works with a RUI device.
 */
void RobeWidgetDetectorTest::testRUIDevice() {
  // software version unlocked (>= 0x14)
  uint8_t info_data[] = {1, 0x14, 3, 0, 0};
  uint8_t uid_data[] = {0x52, 0x53, 1, 0, 0, 10};
  m_endpoint->AddExpectedRobeDataAndReturn(
      INFO_REQUEST_LABEL,
      NULL,
      0,
      INFO_RESPONSE_LABEL,
      info_data,
      sizeof(info_data));
  m_endpoint->AddExpectedRobeDataAndReturn(
      UID_REQUEST_LABEL,
      NULL,
      0,
      UID_RESPONSE_LABEL,
      uid_data,
      sizeof(uid_data));

  m_detector->Discover(&m_descriptor);
  m_ss.Run();

  CPPUNIT_ASSERT(m_found_widget);
  CPPUNIT_ASSERT(!m_failed_widget);

  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(1),
                       m_device_info.hardware_version);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(20),
                       m_device_info.software_version);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(3),
                       m_device_info.eeprom_version);
  CPPUNIT_ASSERT_EQUAL(UID(0x5253, 0x100000a),
                       m_device_info.uid);
}


/*
 * Check that discovery fails with a locked RUI device.
 */
void RobeWidgetDetectorTest::testLockedRUIDevice() {
  // software version locked (0xe)
  uint8_t info_data[] = {1, 0xe, 3, 0, 0};
  uint8_t uid_data[] = {0x52, 0x53, 1, 0, 0, 10};
  m_endpoint->AddExpectedRobeDataAndReturn(
      INFO_REQUEST_LABEL,
      NULL,
      0,
      INFO_RESPONSE_LABEL,
      info_data,
      sizeof(info_data));
  m_endpoint->AddExpectedRobeDataAndReturn(
      UID_REQUEST_LABEL,
      NULL,
      0,
      UID_RESPONSE_LABEL,
      uid_data,
      sizeof(uid_data));

  m_detector->Discover(&m_descriptor);
  m_ss.Run();

  CPPUNIT_ASSERT(!m_found_widget);
  CPPUNIT_ASSERT(m_failed_widget);
}


/*
 * Check that discovery works with a WTX device.
 */
void RobeWidgetDetectorTest::testWTXDevice() {
  uint8_t info_data[] = {1, 2, 3, 0, 0};
  uint8_t uid_data[] = {0x52, 0x53, 2, 0, 0, 10};
  m_endpoint->AddExpectedRobeDataAndReturn(
      INFO_REQUEST_LABEL,
      NULL,
      0,
      INFO_RESPONSE_LABEL,
      info_data,
      sizeof(info_data));
  m_endpoint->AddExpectedRobeDataAndReturn(
      UID_REQUEST_LABEL,
      NULL,
      0,
      UID_RESPONSE_LABEL,
      uid_data,
      sizeof(uid_data));

  m_detector->Discover(&m_descriptor);
  m_ss.Run();

  CPPUNIT_ASSERT(m_found_widget);
  CPPUNIT_ASSERT(!m_failed_widget);

  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(1),
                       m_device_info.hardware_version);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(2),
                       m_device_info.software_version);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(3),
                       m_device_info.eeprom_version);
  CPPUNIT_ASSERT_EQUAL(UID(0x5253, 0x200000a),
                       m_device_info.uid);
}


/*
 * Check that discovery fails for an unknown device.
 */
void RobeWidgetDetectorTest::testUnknownDevice() {
  uint8_t info_data[] = {1, 2, 3, 0, 0};
  uint8_t uid_data[] = {0x52, 0x53, 3, 0, 0, 10};
  m_endpoint->AddExpectedRobeDataAndReturn(
      INFO_REQUEST_LABEL,
      NULL,
      0,
      INFO_RESPONSE_LABEL,
      info_data,
      sizeof(info_data));
  m_endpoint->AddExpectedRobeDataAndReturn(
      UID_REQUEST_LABEL,
      NULL,
      0,
      UID_RESPONSE_LABEL,
      uid_data,
      sizeof(uid_data));

  m_detector->Discover(&m_descriptor);
  m_ss.Run();

  CPPUNIT_ASSERT(!m_found_widget);
  CPPUNIT_ASSERT(m_failed_widget);
}

/**
 * Check a widget that fails to respond
 */
void RobeWidgetDetectorTest::testTimeout() {
  m_endpoint->AddExpectedRobeMessage(INFO_REQUEST_LABEL, NULL, 0);
  m_detector->Discover(&m_descriptor);

  m_ss.Run();
  CPPUNIT_ASSERT(!m_found_widget);
  CPPUNIT_ASSERT(m_failed_widget);
}
