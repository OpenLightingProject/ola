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
#include "ola/network/NetworkUtils.h"
#include "plugins/usbpro/CommonWidgetTest.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/RobeWidgetDetector.h"


using ola::network::ConnectedDescriptor;
using ola::plugin::usbpro::RobeWidget;
using ola::plugin::usbpro::RobeWidgetDetector;
using ola::plugin::usbpro::RobeWidgetInformation;


class RobeWidgetDetectorTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(RobeWidgetDetectorTest);
  CPPUNIT_TEST(testDiscovery);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testDiscovery();
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
  OLA_INFO << "bad";
  CPPUNIT_ASSERT_EQUAL(
      static_cast<ConnectedDescriptor*>(&m_descriptor),
      descriptor);
  m_failed_widget = true;
  m_ss.Terminate();
}


/*
 * Check that discovery works.
 */
void RobeWidgetDetectorTest::testDiscovery() {
  uint8_t info_data[] = {1, 2, 3, 0, 0};
  m_endpoint->AddExpectedRobeDataAndReturn(
      INFO_REQUEST_LABEL,
      NULL,
      0,
      INFO_RESPONSE_LABEL,
      info_data,
      sizeof(info_data));

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
