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
 * @param request_data the data we expect in the request
 * @param request_size the size of the request data
 * @param callback a callback to run when this frame arrives
 */
void MockEndpoint::AddExpectedData(
    const uint8_t *request_data,
    unsigned int request_size,
    NotificationCallback *callback) {
  expected_data call = {
    false,
    false,
    false,
    {request_size, request_data},
    {0, NULL},
    callback
  };
  m_expected_data.push(call);
}


/**
 * Add an expected Usb Pro frame, using the supplied data
 * @param label the message label
 * @param request_payload_data the payload data
 * @param request_payload_size size of the payload data
 * @param callback a callback to run when this frame arrives
 */
void MockEndpoint::AddExpectedUsbProMessage(
    uint8_t label,
    const uint8_t *request_payload_data,
    unsigned int request_payload_size,
    NotificationCallback *callback) {
  unsigned int request_size;
  uint8_t *request = BuildUsbProMessage(label,
                                        request_payload_data,
                                        request_payload_size,
                                        &request_size);
  expected_data call = {
    false,
    true,
    false,
    {request_size, request},
    {0, NULL},
    callback
  };
  m_expected_data.push(call);
}


/**
 * Add an expected Robe frame, using the supplied data
 * @param label the message label
 * @param request_payload_data the payload data
 * @param request_payload_size size of the payload data
 * @param callback a callback to run when this frame arrives
 */
void MockEndpoint::AddExpectedRobeMessage(
    uint8_t label,
    const uint8_t *request_payload_data,
    unsigned int request_payload_size,
    NotificationCallback *callback) {
  unsigned int request_size;
  uint8_t *request = BuildRobeMessage(label,
                                      request_payload_data,
                                      request_payload_size,
                                      &request_size);
  expected_data call = {
    false,
    true,
    false,
    {request_size, request},
    {0, NULL},
    callback
  };
  m_expected_data.push(call);
}


/**
 * Add an expected data frame, and when we get it send a response
 */
void MockEndpoint::AddExpectedDataAndReturn(const uint8_t *request_data,
                                            unsigned int request_size,
                                            const uint8_t *response_data,
                                            unsigned int response_size) {
  expected_data call = {
    true,
    false,
    false,
    {request_size, request_data},
    {response_size, response_data},
    NULL
  };
  m_expected_data.push(call);
}


/**
 * Add an expected Usb Pro frame, using the supplied data. When this arrives
 * return the supplied Usb Pro Frame.
 * @param label the request message label
 * @param request_payload_data the payload data
 * @param response_payload_size size of the payload data
 * @param label the response message label
 * @param response_payload_data the payload data
 * @param response_payload_size size of the payload data
 */
void MockEndpoint::AddExpectedUsbProDataAndReturn(
    uint8_t request_label,
    const uint8_t *request_payload_data,
    unsigned int request_payload_size,
    uint8_t response_label,
    const uint8_t *response_payload_data,
    unsigned int response_payload_size) {
  unsigned int request_size;
  uint8_t *request = BuildUsbProMessage(request_label,
                                        request_payload_data,
                                        request_payload_size,
                                        &request_size);

  unsigned int response_size;
  uint8_t *response = BuildUsbProMessage(response_label,
                                         response_payload_data,
                                         response_payload_size,
                                         &response_size);
  expected_data call = {
    true,
    true,
    true,
    {request_size, request},
    {response_size, response},
    NULL
  };
  m_expected_data.push(call);
}


/**
 * Add an expected Robe frame, using the supplied data. When this arrives
 * return the supplied Robe Frame.
 * @param label the request message label
 * @param request_payload_data the payload data
 * @param response_payload_size size of the payload data
 * @param label the response message label
 * @param response_payload_data the payload data
 * @param response_payload_size size of the payload data
 */
void MockEndpoint::AddExpectedRobeDataAndReturn(
    uint8_t request_label,
    const uint8_t *request_payload_data,
    unsigned int request_payload_size,
    uint8_t response_label,
    const uint8_t *response_payload_data,
    unsigned int response_payload_size) {
  unsigned int request_size;
  uint8_t *request = BuildRobeMessage(request_label,
                                      request_payload_data,
                                      request_payload_size,
                                      &request_size);

  unsigned int response_size;
  uint8_t *response = BuildRobeMessage(response_label,
                                       response_payload_data,
                                       response_payload_size,
                                       &response_size);
  expected_data call = {
    true,
    true,
    true,
    {request_size, request},
    {response_size, response},
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
 * Send an unsolicited Usb Pro message
 */
void MockEndpoint::SendUnsolicitedUsbProData(
    uint8_t response_label,
    const uint8_t *response_payload_data,
    unsigned int response_payload_size) {
  unsigned int response_size;
  uint8_t *response = BuildUsbProMessage(response_label,
                                         response_payload_data,
                                         response_payload_size,
                                         &response_size);

  CPPUNIT_ASSERT(m_descriptor->Send(response, response_size));
  delete[] response;
}


/**
 * Send an unsolicited Robe messages
 */
void MockEndpoint::SendUnsolicitedRobeData(
    uint8_t response_label,
    const uint8_t *response_payload_data,
    unsigned int response_payload_size) {
  unsigned int response_size;
  uint8_t *response = BuildRobeMessage(response_label,
                                       response_payload_data,
                                       response_payload_size,
                                       &response_size);
  CPPUNIT_ASSERT(m_descriptor->Send(response, response_size));
  delete[] response;
}


/**
 * Called when there is new data available. Make sure it matches what we
 * expected and if there is return data send it.
 */
void MockEndpoint::DescriptorReady() {
  CPPUNIT_ASSERT(!m_expected_data.empty());
  expected_data call = m_expected_data.front();
  m_expected_data.pop();

  uint8_t data[call.expected_data_frame.length];
  unsigned int data_received = 0;

  while (data_received != call.expected_data_frame.length) {
    unsigned int offset = data_received;
    if (call.expected_data_frame.length - offset > 100) {
      CPPUNIT_ASSERT(false);
    }

    m_descriptor->Receive(data + offset,
                          call.expected_data_frame.length - offset,
                          data_received);
    data_received += offset;
  }

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

  if (call.free_request)
    delete[] call.expected_data_frame.data;

  if (call.send_response)
    CPPUNIT_ASSERT(m_descriptor->Send(call.return_data_frame.data,
                                      call.return_data_frame.length));

  if (call.callback)
    call.callback->Run();

  if (call.free_response)
    delete[] call.return_data_frame.data;
}


/**
 * Pack data into a Usb Pro style frame.
 * @param label the message label
 * @param data the message data
 * @param data_size the data size
 * @param total_size, pointer which is updated with the message size.
 */
uint8_t *MockEndpoint::BuildUsbProMessage(uint8_t label,
                                          const uint8_t *data,
                                          unsigned int data_size,
                                          unsigned int *total_size) {
  uint8_t *frame = new uint8_t[data_size + HEADER_SIZE + FOOTER_SIZE];
  frame[0] = 0x7e;  // som
  frame[1] = label;
  frame[2] = data_size & 0xff;  // len
  frame[3] = (data_size + 1) >> 8;  // len hi
  memcpy(frame + 4, data, data_size);
  frame[data_size + HEADER_SIZE] = 0xe7;
  *total_size = data_size + HEADER_SIZE + FOOTER_SIZE;
  return frame;
}


/**
 * Pack data into a Robe style frame.
 * @param label the message label
 * @param data the message data
 * @param data_size the data size
 * @param total_size, pointer which is updated with the message size.
 */
uint8_t *MockEndpoint::BuildRobeMessage(uint8_t label,
                                        const uint8_t *data,
                                        unsigned int data_size,
                                        unsigned int *total_size) {
  uint8_t *frame = new uint8_t[data_size + ROBE_HEADER_SIZE +
                               ROBE_FOOTER_SIZE];
  frame[0] = 0xa5;  // som
  frame[1] = label;
  frame[2] = data_size & 0xff;  // len
  frame[3] = (data_size + 1) >> 8;  // len hi

  // header crc
  uint8_t crc = 0;
  for (unsigned int i = 0; i < 4; i++)
    crc += frame[i];
  frame[4] = crc;

  // data & final crc
  crc += frame[4];
  memcpy(frame + ROBE_HEADER_SIZE, data, data_size);
  for (unsigned int i = 0; i < data_size; i++)
    crc += data[i];
  frame[data_size + ROBE_HEADER_SIZE] = crc;
  *total_size = data_size + ROBE_HEADER_SIZE + ROBE_FOOTER_SIZE;
  return frame;
}
