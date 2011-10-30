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
 * UltraDMXProWidgetTest.cpp
 * Test fixture for the UltraDMXProWidget class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "plugins/usbpro/UltraDMXProWidget.h"
#include "plugins/usbpro/CommonWidgetTest.h"


using std::auto_ptr;
using ola::DmxBuffer;


class UltraDMXProWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(UltraDMXProWidgetTest);
  CPPUNIT_TEST(testPrimarySendDMX);
  CPPUNIT_TEST(testSecondarySendDMX);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void testPrimarySendDMX();
    void testSecondarySendDMX();

  private:
    auto_ptr<ola::plugin::usbpro::UltraDMXProWidget> m_widget;

    void Terminate() { m_ss.Terminate(); }

    static const uint8_t PRIMARY_DMX_LABEL = 100;
    static const uint8_t SECONDARY_DMX_LABEL = 101;
};

CPPUNIT_TEST_SUITE_REGISTRATION(UltraDMXProWidgetTest);


void UltraDMXProWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  m_widget.reset(
      new ola::plugin::usbpro::UltraDMXProWidget(&m_ss, &m_descriptor));
}


/**
 * Check that we can send DMX on the primary port
 */
void UltraDMXProWidgetTest::testPrimarySendDMX() {
  // dmx data
  DmxBuffer buffer;
  buffer.SetFromString("0,1,2,3,4");

  // expected message
  uint8_t dmx_frame_data[] = {DMX512_START_CODE, 0, 1, 2, 3, 4};
  m_endpoint->AddExpectedUsbProMessage(
      PRIMARY_DMX_LABEL,
      dmx_frame_data,
      sizeof(dmx_frame_data),
      ola::NewSingleCallback(this, &UltraDMXProWidgetTest::Terminate));

  m_widget->SendDMX(buffer);
  m_ss.Run();
  m_endpoint->Verify();

  // now test an empty frame
  DmxBuffer buffer2;
  uint8_t empty_frame_data[] = {DMX512_START_CODE};  // just the start code
  m_endpoint->AddExpectedUsbProMessage(
      PRIMARY_DMX_LABEL,
      empty_frame_data,
      sizeof(empty_frame_data),
      ola::NewSingleCallback(this, &UltraDMXProWidgetTest::Terminate));
  m_widget->SendDMX(buffer2);
  m_ss.Run();
  m_endpoint->Verify();
}


/**
 * Check that we can send DMX on the secondary port
 */
void UltraDMXProWidgetTest::testSecondarySendDMX() {
  // dmx data
  DmxBuffer buffer;
  buffer.SetFromString("0,1,2,3,4");

  // expected message
  uint8_t dmx_frame_data[] = {DMX512_START_CODE, 0, 1, 2, 3, 4};
  m_endpoint->AddExpectedUsbProMessage(
      SECONDARY_DMX_LABEL,
      dmx_frame_data,
      sizeof(dmx_frame_data),
      ola::NewSingleCallback(this, &UltraDMXProWidgetTest::Terminate));

  m_widget->SendSecondaryDMX(buffer);
  m_ss.Run();
  m_endpoint->Verify();

  // now test an empty frame
  DmxBuffer buffer2;
  uint8_t empty_frame_data[] = {DMX512_START_CODE};  // just the start code
  m_endpoint->AddExpectedUsbProMessage(
      SECONDARY_DMX_LABEL,
      empty_frame_data,
      sizeof(empty_frame_data),
      ola::NewSingleCallback(this, &UltraDMXProWidgetTest::Terminate));
  m_widget->SendSecondaryDMX(buffer2);
  m_ss.Run();
  m_endpoint->Verify();
}
