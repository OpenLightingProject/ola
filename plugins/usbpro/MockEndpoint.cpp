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
 * MockEndpoint.cpp
 * This allows unittest of data received on a ConnectedDescriptor.
 * Copyright (C) 2011 Simon Newton
 */

#include <string.h>
#include <cppunit/extensions/HelperMacros.h>
#include <queue>

#include "ola/Callback.h"
#include "ola/Logging.h"
#include "plugins/usbpro/MockEndpoint.h"


MockEndpoint::MockEndpoint(ola::network::ConnectedDescriptor *descriptor)
    : m_descriptor(descriptor) {
  m_descriptor->SetOnData(
            ola::NewCallback(this, &MockEndpoint::DescriptorReady));
}


MockEndpoint::~MockEndpoint() {
  m_descriptor->SetOnData(NULL);
}


/**
 * Add an expected data frame to the queue.
 */
void MockEndpoint::AddExpectedData(
    const uint8_t *data,
    unsigned int length,
    NotificationCallback *callback) {
  expected_data call = {
    false,
    {length, data},
    {0, NULL},
    callback
  };
  m_expected_data.push(call);
}


/**
 * Add an expected data frame, and when we get it send a response
 */
void MockEndpoint::AddExpectedDataAndReturn(const uint8_t *data,
                                            unsigned int length,
                                            const uint8_t *return_data,
                                            unsigned int return_length) {
  expected_data call = {
    true,
    {length, data},
    {return_length, return_data},
    NULL
  };
  m_expected_data.push(call);
}


/**
 * Send some data from this endpoint without having first received anything
 */
void MockEndpoint::SendUnsolicited(const uint8_t *data,
                                   unsigned int length) {
  CPPUNIT_ASSERT(m_descriptor->Send(data, length));
}


/**
 * Called when there is new data available. Make sure it matches what we
 * expected and if there is return data send it.
 */
void MockEndpoint::DescriptorReady() {
  uint8_t data[MAX_DATA_SIZE];
  unsigned int data_received;
  m_descriptor->Receive(data, sizeof(data), data_received);

  CPPUNIT_ASSERT(!m_expected_data.empty());
  expected_data call = m_expected_data.front();
  m_expected_data.pop();
  CPPUNIT_ASSERT_EQUAL(call.expected_data_frame.length, data_received);
  bool data_matches = !memcmp(call.expected_data_frame.data,
                              data,
                              data_received);
  if (!data_matches) {
    for (unsigned int i = 0; i < data_received; ++i)
      OLA_WARN << i << ": 0x" << std::hex << static_cast<int>(data[i]) << " 0x"
        << static_cast<int>(call.expected_data_frame.data[i]);
  }
  CPPUNIT_ASSERT(data_matches);

  if (call.send_response)
    CPPUNIT_ASSERT(m_descriptor->Send(call.return_data_frame.data,
                                      call.return_data_frame.length));

  if (call.callback)
    call.callback->Run();
}
