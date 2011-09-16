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
 * MockUsbWidget.h
 * The mock usb widget used for testing
 * Copyright (C) 2010 Simon Newton
 */

#ifndef PLUGINS_USBPRO_MOCKUSBWIDGET_H_
#define PLUGINS_USBPRO_MOCKUSBWIDGET_H_

#include <cppunit/extensions/HelperMacros.h>
#include <queue>

#include "ola/Callback.h"
#include "plugins/usbpro/BaseUsbProWidget.h"


/**
 * The MockUsbWidget, used to verify calls.
 */
class MockUsbWidget: public ola::plugin::usbpro::BaseUsbProWidget {
  public:
    MockUsbWidget()
        : BaseUsbProWidget(NULL),
          m_callback(NULL),
          m_descriptor_closed(false) {}
    ~MockUsbWidget() {
      if (m_callback)
        delete m_callback;
    }

    ola::network::ConnectedDescriptor *GetDescriptor() const {
      return NULL;
    }

    void SetMessageHandler(
        ola::Callback3<void, uint8_t, const uint8_t*,
                       unsigned int> *callback) {
      if (m_callback)
        delete m_callback;
      m_callback = callback;
    }

    // this doesn't do anything
    void SetOnRemove(ola::SingleUseCallback0<void> *on_close) {
      delete on_close;
    }

    void CloseDescriptor() {
      m_descriptor_closed = true;
    }

    bool SendMessage(uint8_t label,
                     const uint8_t *data,
                     unsigned int length) const;

    void AddExpectedCall(uint8_t expected_label,
                         const uint8_t *expected_data,
                         unsigned int expected_length);

    void AddExpectedCall(uint8_t expected_label,
                         const uint8_t *expected_data,
                         unsigned int expected_length,
                         uint8_t return_label,
                         const uint8_t *return_data,
                         unsigned int return_length);

    void SendUnsolicited(uint8_t label,
                         const uint8_t *data,
                         unsigned int length);

    void Verify() {
      CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), m_expected_calls.size());
    }

    bool IsClosed() const { return m_descriptor_closed; }

  private:
    ola::Callback3<void, uint8_t, const uint8_t*, unsigned int> *m_callback;
    bool m_descriptor_closed;

    typedef struct {
      uint8_t label;
      unsigned int length;
      const uint8_t *data;
    } command;

    typedef struct {
      bool send_response;
      command expected_command;
      command return_command;
    } expected_call;

    mutable std::queue<expected_call> m_expected_calls;
};
#endif  // PLUGINS_USBPRO_MOCKUSBWIDGET_H_
