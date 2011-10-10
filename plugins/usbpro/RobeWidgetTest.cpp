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
 * RobeWidgetTest.cpp
 * Test fixture for the DmxterWidget class
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <queue>
#include <string>
#include <vector>

#include "ola/BaseTypes.h"
#include "ola/Callback.h"
#include "ola/DmxBuffer.h"
#include "ola/Logging.h"
#include "ola/rdm/UID.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/CommonWidgetTest.h"


using ola::DmxBuffer;
using ola::plugin::usbpro::RobeWidget;
using std::auto_ptr;
using std::string;


class RobeWidgetTest: public CommonWidgetTest {
  CPPUNIT_TEST_SUITE(RobeWidgetTest);
  CPPUNIT_TEST(testSendDMX);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();

    void testSendDMX();

  private:
    auto_ptr<ola::plugin::usbpro::RobeWidget> m_widget;

    void Terminate() {
      m_ss.Terminate();
    }

    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 5;
    static const uint8_t DMX_FRAME_LABEL = 0x06;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RobeWidgetTest);


void RobeWidgetTest::setUp() {
  CommonWidgetTest::setUp();
  ola::rdm::UID uid(0, 1);
  m_widget.reset(
      new ola::plugin::usbpro::RobeWidget(&m_descriptor, &m_ss, uid));
}


/*
 * Check that we can send DMX
 */
void RobeWidgetTest::testSendDMX() {
  // dmx data
  DmxBuffer buffer;
  buffer.SetFromString("0,1,2,3,4");

  // expected message
  uint8_t dmx_frame_data[] = {0, 1, 2, 3, 4, 0, 0, 0, 0};
  // add the expected data, run and verify.
  m_endpoint->AddExpectedRobeMessage(
      DMX_FRAME_LABEL,
      dmx_frame_data,
      sizeof(dmx_frame_data),
      ola::NewSingleCallback(this, &RobeWidgetTest::Terminate));
  m_widget->SendDMX(buffer);
  m_ss.Run();
  m_endpoint->Verify();

  // now test an empty frame
  DmxBuffer buffer2;
  uint8_t empty_frame_data[] = {0, 0, 0, 0};  // null frames still have 4 bytes

  // add the expected data, run and verify.
  m_endpoint->AddExpectedRobeMessage(
      DMX_FRAME_LABEL,
      empty_frame_data,
      sizeof(empty_frame_data),
      ola::NewSingleCallback(this, &RobeWidgetTest::Terminate));
  m_widget->SendDMX(buffer2);
  m_ss.Run();
  m_endpoint->Verify();
}

