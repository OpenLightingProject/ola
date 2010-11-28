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
 * MockUsbWidget.cpp
 * The mock usb widget used for testing
 * Copyright (C) 2010 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <queue>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "plugins/usbpro/MockUsbWidget.h"

bool MockUsbWidget::SendMessage(uint8_t label,
                                const uint8_t *data,
                                unsigned int length) const {
  CPPUNIT_ASSERT(m_expected_calls.size());
  expected_call call = m_expected_calls.front();
  m_expected_calls.pop();
  CPPUNIT_ASSERT_EQUAL(call.expected_command.label, label);
  CPPUNIT_ASSERT_EQUAL(call.expected_command.length, length);
  CPPUNIT_ASSERT(!memcmp(call.expected_command.data, data, length));

  if (call.send_response)
    m_callback->Run(call.return_command.label,
                    call.return_command.data,
                    call.return_command.length);
  return true;
}


/**
 * A Call that doesn't trigger a response
 */
void MockUsbWidget::AddExpectedCall(uint8_t expected_label,
                                    const uint8_t *expected_data,
                                    unsigned int expected_length) {
  expected_call call;
  call.send_response = false;
  call.expected_command.label = expected_label;
  call.expected_command.data = expected_data;
  call.expected_command.length = expected_length;
  m_expected_calls.push(call);
}

/**
 * A Call that triggers a response
 */
void MockUsbWidget::AddExpectedCall(uint8_t expected_label,
                                    const uint8_t *expected_data,
                                    unsigned int expected_length,
                                    uint8_t return_label,
                                    const uint8_t *return_data,
                                    unsigned int return_length) {
  expected_call call;
  call.send_response = true;
  call.expected_command.label = expected_label;
  call.expected_command.data = expected_data;
  call.expected_command.length = expected_length;
  call.return_command.label = return_label;
  call.return_command.data = return_data;
  call.return_command.length = return_length;
  m_expected_calls.push(call);
}
