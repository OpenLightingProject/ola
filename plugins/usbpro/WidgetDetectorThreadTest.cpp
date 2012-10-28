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
 * WidgetDetectorThreadTest.cpp
 * Test fixture for the RobeWidgetDetector class
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>
#include <string>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"
#include "ola/rdm/UID.h"
#include "plugins/usbpro/ArduinoWidget.h"
#include "plugins/usbpro/DmxTriWidget.h"
#include "plugins/usbpro/DmxterWidget.h"
#include "plugins/usbpro/EnttecUsbProWidget.h"
#include "plugins/usbpro/MockEndpoint.h"
#include "plugins/usbpro/BaseRobeWidget.h"
#include "plugins/usbpro/RobeWidget.h"
#include "plugins/usbpro/UltraDMXProWidget.h"
#include "plugins/usbpro/WidgetDetectorThread.h"
#include "ola/testing/TestUtils.h"


using ola::io::ConnectedDescriptor;
using ola::io::UnixSocket;
using ola::plugin::usbpro::ArduinoWidget;
using ola::plugin::usbpro::BaseRobeWidget;
using ola::plugin::usbpro::BaseUsbProWidget;
using ola::plugin::usbpro::DmxTriWidget;
using ola::plugin::usbpro::DmxterWidget;
using ola::plugin::usbpro::EnttecUsbProWidget;
using ola::plugin::usbpro::NewWidgetHandler;
using ola::plugin::usbpro::RobeWidget;
using ola::plugin::usbpro::RobeWidgetInformation;
using ola::plugin::usbpro::UltraDMXProWidget;
using ola::plugin::usbpro::UsbProWidgetInformation;
using ola::plugin::usbpro::WidgetDetectorThread;
using ola::rdm::UID;
using std::auto_ptr;
using std::string;


/**
 * This overrides RunScan so we don't attempt to open devices on the local
 * machine.
 */
class MockWidgetDetectorThread: public WidgetDetectorThread {
  public:
    MockWidgetDetectorThread(NewWidgetHandler *widget_handler,
                             ola::io::SelectServerInterface *ss)
          // set very short timeouts since this is a unittest
        : WidgetDetectorThread(widget_handler, ss, 10, 10),
          m_descriptor(new ola::io::UnixSocket()) {
      m_descriptor->Init();
    }

    UnixSocket* GetOtherEnd() { return m_descriptor->OppositeEnd(); }

  protected:
    bool RunScan() {
      PerformDiscovery("/mock_device", m_descriptor);
      return true;
    }

  private:
    UnixSocket *m_descriptor;
};


class WidgetDetectorThreadTest: public CppUnit::TestFixture,
                                public NewWidgetHandler {
  CPPUNIT_TEST_SUITE(WidgetDetectorThreadTest);
  CPPUNIT_TEST(testArduinoWidget);
  CPPUNIT_TEST(testDmxTriWidget);
  CPPUNIT_TEST(testDmxterWidget);
  CPPUNIT_TEST(testUsbProWidget);
  CPPUNIT_TEST(testRobeWidget);
  CPPUNIT_TEST(testUltraDmxWidget);
  CPPUNIT_TEST(testTimeout);
  CPPUNIT_TEST(testClose);
  CPPUNIT_TEST_SUITE_END();

  public:
    void setUp();
    void tearDown();

    void testArduinoWidget();
    void testDmxTriWidget();
    void testDmxterWidget();
    void testUsbProWidget();
    void testRobeWidget();
    void testUltraDmxWidget();
    void testTimeout();
    void testClose();

  private:
    ola::io::SelectServer m_ss;
    auto_ptr<MockEndpoint> m_endpoint;
    auto_ptr<MockWidgetDetectorThread> m_thread;
    auto_ptr<ola::io::UnixSocket> m_other_end;

    typedef enum {
      NONE,
      ARDUINO,
      ENTTEC,
      DMX_TRI,
      DMXTER,
      ROBE,
      ULTRA_DMX,
    } WidgetType;

    WidgetType m_received_widget_type;

    void Timeout() { m_ss.Terminate(); }
    // widget handlers follow
    void NewWidget(ArduinoWidget *widget,
                   const UsbProWidgetInformation &information) {
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x7a70),
                           information.esta_id);
      OLA_ASSERT_EQ(string("Open Lighting"), information.manufacturer);
      OLA_ASSERT_EQ(static_cast<uint16_t>(1),
                           information.device_id);
      OLA_ASSERT_EQ(string("Unittest Device"), information.device);
      OLA_ASSERT_EQ(static_cast<uint32_t>(0x12345678),
                           information.serial);
      m_thread->FreeWidget(widget);
      m_received_widget_type = ARDUINO;
      m_ss.Terminate();
    }
    void NewWidget(EnttecUsbProWidget *widget,
                   const UsbProWidgetInformation &information) {
      OLA_ASSERT_EQ(static_cast<uint16_t>(0),
                           information.esta_id);
      OLA_ASSERT_EQ(string(""), information.manufacturer);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0),
                           information.device_id);
      OLA_ASSERT_EQ(string(""), information.device);
      OLA_ASSERT_EQ(static_cast<uint32_t>(0x12345678),
                           information.serial);
      m_thread->FreeWidget(widget);
      m_received_widget_type = ENTTEC;
      m_ss.Terminate();
    }
    void NewWidget(DmxTriWidget *widget,
                   const UsbProWidgetInformation &information) {
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x6864),
                           information.esta_id);
      OLA_ASSERT_EQ(string("JESE"), information.manufacturer);
      OLA_ASSERT_EQ(static_cast<uint16_t>(2),
                           information.device_id);
      OLA_ASSERT_EQ(string("RDM-TRI"), information.device);
      OLA_ASSERT_EQ(static_cast<uint32_t>(0x12345678),
                           information.serial);
      m_thread->FreeWidget(widget);
      m_received_widget_type = DMX_TRI;
      m_ss.Terminate();
    }
    void NewWidget(DmxterWidget *widget,
                   const UsbProWidgetInformation &information) {
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x4744),
                           information.esta_id);
      OLA_ASSERT_EQ(string("Goddard Design"), information.manufacturer);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x444d),
                           information.device_id);
      OLA_ASSERT_EQ(string("DMXter4"), information.device);
      OLA_ASSERT_EQ(static_cast<uint32_t>(0x12345678),
                           information.serial);
      m_thread->FreeWidget(widget);
      m_received_widget_type = DMXTER;
      m_ss.Terminate();
    }
    void NewWidget(RobeWidget *widget,
                   const RobeWidgetInformation &information) {
      m_thread->FreeWidget(widget);
      OLA_ASSERT_EQ(static_cast<uint8_t>(1),
                           information.hardware_version);
      OLA_ASSERT_EQ(static_cast<uint8_t>(11),
                           information.software_version);
      OLA_ASSERT_EQ(static_cast<uint8_t>(3),
                           information.eeprom_version);
      OLA_ASSERT_EQ(UID(0x5253, 0x200000a),
                           information.uid);
      m_received_widget_type = ROBE;
      m_ss.Terminate();
    }
    void NewWidget(UltraDMXProWidget *widget,
                   const UsbProWidgetInformation &information) {
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x6a6b),
                           information.esta_id);
      OLA_ASSERT_EQ(string("DMXking.com"), information.manufacturer);
      OLA_ASSERT_EQ(static_cast<uint16_t>(0x2),
                           information.device_id);
      OLA_ASSERT_EQ(string("ultraDMX Pro"), information.device);
      OLA_ASSERT_EQ(static_cast<uint32_t>(0x12345678),
                           information.serial);
      m_thread->FreeWidget(widget);
      m_received_widget_type = ULTRA_DMX;
      m_ss.Terminate();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(WidgetDetectorThreadTest);



void WidgetDetectorThreadTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_received_widget_type = NONE;
  m_thread.reset(new MockWidgetDetectorThread(this, &m_ss));

  m_ss.RegisterSingleTimeout(
      100,  // this should take at most 40ms
      ola::NewSingleCallback(this, &WidgetDetectorThreadTest::Timeout));

  m_other_end.reset(m_thread->GetOtherEnd());
  m_ss.AddReadDescriptor(m_other_end.get());
  m_endpoint.reset(new MockEndpoint(m_other_end.get()));
}


void WidgetDetectorThreadTest::tearDown() {
  m_thread->Join(NULL);
  m_endpoint->Verify();
  m_ss.RemoveReadDescriptor(m_other_end.get());
  m_endpoint.reset();
}


/**
 * Check that we can locate a Arduino widget
 */
void WidgetDetectorThreadTest::testArduinoWidget() {
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  uint8_t manufacturer_data[] = "pzOpen Lighting";
  uint8_t device_data[] = "\001\000Unittest Device";
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::MANUFACTURER_LABEL,
      NULL,
      0,
      BaseUsbProWidget::MANUFACTURER_LABEL,
      manufacturer_data,
      sizeof(manufacturer_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::DEVICE_LABEL,
      NULL,
      0,
      BaseUsbProWidget::DEVICE_LABEL,
      device_data,
      sizeof(device_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::SERIAL_LABEL,
      NULL,
      0,
      BaseUsbProWidget::SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_thread->Start();
  m_thread->WaitUntilRunning();
  m_ss.Run();
  OLA_ASSERT_EQ(ARDUINO, m_received_widget_type);
}


/**
 * Check that we can locate a DMX-TRI widget
 */
void WidgetDetectorThreadTest::testDmxTriWidget() {
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  uint8_t manufacturer_data[] = "\144\150JESE";
  uint8_t device_data[] = "\002\000RDM-TRI";
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::MANUFACTURER_LABEL,
      NULL,
      0,
      BaseUsbProWidget::MANUFACTURER_LABEL,
      manufacturer_data,
      sizeof(manufacturer_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::DEVICE_LABEL,
      NULL,
      0,
      BaseUsbProWidget::DEVICE_LABEL,
      device_data,
      sizeof(device_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::SERIAL_LABEL,
      NULL,
      0,
      BaseUsbProWidget::SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_thread->Start();
  m_thread->WaitUntilRunning();
  m_ss.Run();
  OLA_ASSERT_EQ(DMX_TRI, m_received_widget_type);
}


/**
 * Check that we can locate a Arduino widget
 */
void WidgetDetectorThreadTest::testDmxterWidget() {
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  uint8_t manufacturer_data[] = "\104\107Goddard Design";
  uint8_t device_data[] = "\115\104DMXter4";
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::MANUFACTURER_LABEL,
      NULL,
      0,
      BaseUsbProWidget::MANUFACTURER_LABEL,
      manufacturer_data,
      sizeof(manufacturer_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::DEVICE_LABEL,
      NULL,
      0,
      BaseUsbProWidget::DEVICE_LABEL,
      device_data,
      sizeof(device_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::SERIAL_LABEL,
      NULL,
      0,
      BaseUsbProWidget::SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_thread->Start();
  m_thread->WaitUntilRunning();
  m_ss.Run();
  OLA_ASSERT_EQ(DMXTER, m_received_widget_type);
}


/**
 * Check that we can locate a Usb Pro widget
 */
void WidgetDetectorThreadTest::testUsbProWidget() {
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::MANUFACTURER_LABEL,
                                       NULL,
                                       0);
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::DEVICE_LABEL,
                                       NULL,
                                       0);
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::SERIAL_LABEL,
      NULL,
      0,
      BaseUsbProWidget::SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_thread->Start();
  m_thread->WaitUntilRunning();
  m_ss.Run();
  OLA_ASSERT_EQ(ENTTEC, m_received_widget_type);
}


/**
 * Check that we can locate a Robe widget.
 */
void WidgetDetectorThreadTest::testRobeWidget() {
  // usb pro messages first
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::MANUFACTURER_LABEL,
                                       NULL,
                                       0);
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::DEVICE_LABEL,
                                       NULL,
                                       0);
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::SERIAL_LABEL,
                                       NULL,
                                       0);

  // robe messages
  uint8_t info_data[] = {1, 11, 3, 0, 0};
  uint8_t uid_data[] = {0x52, 0x53, 2, 0, 0, 10};
  m_endpoint->AddExpectedRobeDataAndReturn(
      BaseRobeWidget::INFO_REQUEST,
      NULL,
      0,
      BaseRobeWidget::INFO_RESPONSE,
      info_data,
      sizeof(info_data));
  m_endpoint->AddExpectedRobeDataAndReturn(
      BaseRobeWidget::UID_REQUEST,
      NULL,
      0,
      BaseRobeWidget::UID_RESPONSE,
      uid_data,
      sizeof(uid_data));

  m_thread->Start();
  m_thread->WaitUntilRunning();
  m_ss.Run();
  OLA_ASSERT_EQ(ROBE, m_received_widget_type);
}


/**
 * Check that we can locate an Ultra DMX widget.
 */
void WidgetDetectorThreadTest::testUltraDmxWidget() {
  uint8_t serial_data[] = {0x78, 0x56, 0x34, 0x12};
  uint8_t manufacturer_data[] = "\153\152DMXking.com";
  uint8_t device_data[] = "\002\000ultraDMX Pro";
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::MANUFACTURER_LABEL,
      NULL,
      0,
      BaseUsbProWidget::MANUFACTURER_LABEL,
      manufacturer_data,
      sizeof(manufacturer_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::DEVICE_LABEL,
      NULL,
      0,
      BaseUsbProWidget::DEVICE_LABEL,
      device_data,
      sizeof(device_data));
  m_endpoint->AddExpectedUsbProDataAndReturn(
      BaseUsbProWidget::SERIAL_LABEL,
      NULL,
      0,
      BaseUsbProWidget::SERIAL_LABEL,
      serial_data,
      sizeof(serial_data));

  m_thread->Start();
  m_thread->WaitUntilRunning();
  m_ss.Run();
  OLA_ASSERT_EQ(ULTRA_DMX, m_received_widget_type);
}


/**
 * Check a widget that fails to respond
 */
void WidgetDetectorThreadTest::testTimeout() {
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::MANUFACTURER_LABEL,
                                       NULL,
                                       0);
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::DEVICE_LABEL,
                                       NULL,
                                       0);
  m_endpoint->AddExpectedUsbProMessage(BaseUsbProWidget::SERIAL_LABEL,
                                       NULL,
                                       0);
  m_endpoint->AddExpectedRobeMessage(BaseRobeWidget::INFO_REQUEST,
                                     NULL,
                                     0);
  m_thread->Start();
  m_thread->WaitUntilRunning();
  m_ss.Run();
  OLA_ASSERT_EQ(NONE, m_received_widget_type);
}


/**
 * Check we handle the case when a widget disappears.
 */
void WidgetDetectorThreadTest::testClose() {
  m_ss.RemoveReadDescriptor(m_other_end.get());
  m_thread->Start();
  m_thread->WaitUntilRunning();
  // possible race condition here
  m_other_end->Close();
  m_ss.Run();
  OLA_ASSERT_EQ(NONE, m_received_widget_type);
}
