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

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/network/NetworkUtils.h"
#include "ola/network/SelectServer.h"
#include "ola/network/Socket.h"
#include "plugins/usbpro/UsbWidget.h"
#include "plugins/usbpro/UsbProWidgetDetector.h"


using ola::network::ConnectedDescriptor;
using ola::network::PipeDescriptor;
using ola::plugin::usbpro::WidgetInformation;
using ola::plugin::usbpro::UsbWidget;
using ola::plugin::usbpro::UsbProWidgetDetector;
using std::string;


class UsbProWidgetDetectorTest: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(UsbProWidgetDetectorTest);
  CPPUNIT_TEST(testExtendedDiscovery);
  CPPUNIT_TEST(testDiscovery);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testExtendedDiscovery();
    void testDiscovery();
    void testTimeout();

  private:
    ola::network::SelectServer m_ss;
    UsbProWidgetDetector *m_detector;
    PipeDescriptor m_descriptor;
    PipeDescriptor *m_other_end;
    UsbWidget *m_widget;
    UsbWidget *m_responder;
    WidgetInformation m_device_info;
    bool m_found_widget;
    bool m_failed_widget;
    bool m_send_manufacturer;
    bool m_send_device;
    bool m_send_serial;

    void NewWidget(UsbWidget *widget, const WidgetInformation *info);
    void FailedWidget(UsbWidget *widget);

    void ResponderHandler(uint8_t label,
                          const uint8_t *data,
                          unsigned int size);
    void SendMessage(uint8_t label, uint16_t id, const string &description);
    void Timeout() { m_ss.Terminate(); }

    static const uint32_t SERIAL;
    static const uint16_t MANUFACTURER_ID;
    static const uint16_t DEVICE_ID;
    static const char MANUFACTURER_NAME[];
    static const char DEVICE_NAME[];
};


CPPUNIT_TEST_SUITE_REGISTRATION(UsbProWidgetDetectorTest);

const uint32_t UsbProWidgetDetectorTest::SERIAL = 0x12345678;
const uint16_t UsbProWidgetDetectorTest::MANUFACTURER_ID = 0x7a70;
const uint16_t UsbProWidgetDetectorTest::DEVICE_ID = 0x2010;
const char UsbProWidgetDetectorTest::MANUFACTURER_NAME[] = "Open Lighting";
const char UsbProWidgetDetectorTest::DEVICE_NAME[] = "Unittest Device";


void UsbProWidgetDetectorTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_found_widget = false;
  m_failed_widget = false;
  m_send_manufacturer = true;
  m_send_device = true;
  m_send_serial = true;
  m_detector = new UsbProWidgetDetector(
      &m_ss,
      ola::NewCallback(this, &UsbProWidgetDetectorTest::NewWidget),
      ola::NewCallback(this, &UsbProWidgetDetectorTest::FailedWidget),
      10);
  m_descriptor.Init();
  m_other_end = m_descriptor.OppositeEnd();

  m_ss.AddReadDescriptor(&m_descriptor);
  m_ss.AddReadDescriptor(m_other_end, true);
  m_widget = new UsbWidget(&m_descriptor);

  // we fake another widget at the other end of the pipe
  m_responder = new UsbWidget(m_other_end);
  m_responder->SetMessageHandler(
      ola::NewCallback(this, &UsbProWidgetDetectorTest::ResponderHandler));

  m_ss.RegisterSingleTimeout(
      40,  // 40ms should be enough
      ola::NewSingleCallback(this, &UsbProWidgetDetectorTest::Timeout));
}


void UsbProWidgetDetectorTest::tearDown() {
  delete m_detector;
  delete m_widget;
  delete m_responder;
}


void UsbProWidgetDetectorTest::NewWidget(UsbWidget *widget,
                                   const WidgetInformation *info) {
  CPPUNIT_ASSERT_EQUAL(m_widget, widget);
  m_found_widget = true;
  m_device_info = *info;
  m_ss.Terminate();
  delete info;
}


void UsbProWidgetDetectorTest::FailedWidget(UsbWidget *widget) {
  CPPUNIT_ASSERT_EQUAL(m_widget, widget);
  m_failed_widget = true;
  m_ss.Terminate();
}


/**
 * Called when a new message arrives
 */
void UsbProWidgetDetectorTest::ResponderHandler(uint8_t label,
                                          const uint8_t *data,
                                          unsigned int size) {
  if (label == UsbWidget::MANUFACTURER_LABEL) {
    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(0), size);
    CPPUNIT_ASSERT_EQUAL(static_cast<const uint8_t*>(NULL), data);

    if (m_send_manufacturer)
      SendMessage(label, MANUFACTURER_ID, MANUFACTURER_NAME);

  } else if (label == UsbWidget::DEVICE_LABEL) {
    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(0), size);
    CPPUNIT_ASSERT_EQUAL(static_cast<const uint8_t*>(NULL), data);

    if (m_send_device)
      SendMessage(label, DEVICE_ID, DEVICE_NAME);

  } else if (label == UsbWidget::SERIAL_LABEL) {
    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned int>(0), size);
    CPPUNIT_ASSERT_EQUAL(static_cast<const uint8_t*>(NULL), data);

    if (m_send_serial) {
      uint32_t serial = ola::network::HostToLittleEndian(SERIAL);
      m_responder->SendMessage(UsbWidget::SERIAL_LABEL,
                               reinterpret_cast<const uint8_t*>(&serial),
                               sizeof(serial));
    }
  }
}


void UsbProWidgetDetectorTest::SendMessage(uint8_t label,
                                     uint16_t id,
                                     const string &description) {
    struct {
      uint16_t id;
      char name[32];
    } response;
    response.id = ola::network::HostToLittleEndian(id);
    strncpy(response.name, description.data(), sizeof(response.name));
    m_responder->SendMessage(label,
                             reinterpret_cast<uint8_t*>(&response),
                             sizeof(response));
}


/**
 * Test that discovery works for a device that implements the extended set of
 * messages.
 */
void UsbProWidgetDetectorTest::testExtendedDiscovery() {
  m_detector->Discover(m_widget);
  m_ss.Run();

  CPPUNIT_ASSERT(m_found_widget);
  CPPUNIT_ASSERT(!m_failed_widget);

  CPPUNIT_ASSERT_EQUAL(MANUFACTURER_ID, m_device_info.esta_id);
  CPPUNIT_ASSERT_EQUAL(DEVICE_ID, m_device_info.device_id);
  CPPUNIT_ASSERT_EQUAL(string(MANUFACTURER_NAME), m_device_info.manufactuer);
  CPPUNIT_ASSERT_EQUAL(string(DEVICE_NAME), m_device_info.device);
  CPPUNIT_ASSERT_EQUAL(SERIAL, m_device_info.serial);
}


/**
 * Check that discovery works for a device that just implements the serial #
 */
void UsbProWidgetDetectorTest::testDiscovery() {
  m_send_manufacturer = false;
  m_send_device = false;
  m_detector->Discover(m_widget);
  m_ss.Run();

  CPPUNIT_ASSERT(m_found_widget);
  CPPUNIT_ASSERT(!m_failed_widget);

  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(0), m_device_info.esta_id);
  CPPUNIT_ASSERT_EQUAL(static_cast<uint16_t>(0), m_device_info.device_id);
  CPPUNIT_ASSERT_EQUAL(string(), m_device_info.manufactuer);
  CPPUNIT_ASSERT_EQUAL(string(), m_device_info.device);
  CPPUNIT_ASSERT_EQUAL(SERIAL, m_device_info.serial);
}


/**
 * Check a widget that fails to respond
 */
void UsbProWidgetDetectorTest::testTimeout() {
  m_send_manufacturer = false;
  m_send_device = false;
  m_send_serial = false;
  m_detector->Discover(m_widget);
  m_ss.Run();

  CPPUNIT_ASSERT(!m_found_widget);
  CPPUNIT_ASSERT(m_failed_widget);
}
