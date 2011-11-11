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
 * MockEndpoint.h
 * This allows unittest of data received on a ConnectedDescriptor. The general
 * use case is:
 *
 * ola::network::PipeSocket pipe;
 * pipe.Init();
 * ola::network::PipeSocket *other_end = pip.OppositeEnd();
 * MockEndpoint endpoint(other_end)
 * ola::network::SelectServer ss;
 * ss.AddReadDescriptor(&pipe);
 * ss.AddReadDescriptor(other_end);
 * // Do the test here
 * ss.Run();
 * endpoint.Verify();  // make sure there are no calls remaining
 * Copyright (C) 2011 Simon Newton
 */

#ifndef PLUGINS_USBPRO_MOCKENDPOINT_H_
#define PLUGINS_USBPRO_MOCKENDPOINT_H_

#include <cppunit/extensions/HelperMacros.h>
#include <queue>

#include "ola/Callback.h"
#include "ola/network/Socket.h"


/**
 * The MockEndpoint, this is used for the unittests.
 */
class MockEndpoint {
  public:
    explicit MockEndpoint(ola::network::ConnectedDescriptor *descriptor);
    ~MockEndpoint();

    typedef ola::SingleUseCallback0<void> NotificationCallback;

    void AddExpectedData(const uint8_t *request_data,
                         unsigned int request_size,
                         NotificationCallback *callback = NULL);

    // This does the same as above, but puts the data inside a Usb Pro style
    // frame.
    void AddExpectedUsbProMessage(uint8_t label,
                                  const uint8_t *request_payload_data,
                                  unsigned int request_payload_size,
                                  NotificationCallback *callback = NULL);

    // This does the same as above, but puts the data inside a Robe style
    // frame.
    void AddExpectedRobeMessage(uint8_t label,
                                const uint8_t *request_payload_data,
                                unsigned int request_payload_size,
                                NotificationCallback *callback = NULL);

    void AddExpectedDataAndReturn(const uint8_t *request_data,
                                  unsigned int request_size,
                                  const uint8_t *response_data,
                                  unsigned int response_size);

    // This does the same as above, but puts the data inside a Usb Pro style
    // frame.
    void AddExpectedUsbProDataAndReturn(uint8_t request_label,
                                        const uint8_t *request_payload_data,
                                        unsigned int request_payload_size,
                                        uint8_t response_label,
                                        const uint8_t *response_payload_data,
                                        unsigned int response_payload_size);

    // This does the same as above, but puts the data inside a Robe style
    // frame.
    void AddExpectedRobeDataAndReturn(uint8_t request_label,
                                      const uint8_t *request_payload_data,
                                      unsigned int request_payload_size,
                                      uint8_t response_label,
                                      const uint8_t *response_payload_data,
                                      unsigned int response_payload_size);

    void SendUnsolicited(const uint8_t *data,
                         unsigned int length);

    void SendUnsolicitedUsbProData(uint8_t label,
                                   const uint8_t *response_payload_data,
                                   unsigned int response_payload_size);

    void SendUnsolicitedRobeData(uint8_t label,
                                 const uint8_t *response_payload_data,
                                 unsigned int response_payload_size);

    void Verify() {
      CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), m_expected_data.size());
    }

  private:
    ola::network::ConnectedDescriptor *m_descriptor;

    typedef struct {
      unsigned int length;
      const uint8_t *data;
    } data_frame;

    typedef struct {
      bool send_response;
      bool free_request;
      bool free_response;
      data_frame expected_data_frame;
      data_frame return_data_frame;
      NotificationCallback *callback;
    } expected_data;

    std::queue<expected_data> m_expected_data;

    void DescriptorReady();
    uint8_t *BuildUsbProMessage(uint8_t label,
                                const uint8_t *data,
                                unsigned int data_size,
                                unsigned int *total_size);

    uint8_t *BuildRobeMessage(uint8_t label,
                              const uint8_t *data,
                              unsigned int data_size,
                              unsigned int *total_size);

    enum {MAX_DATA_SIZE = 600};
    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 4;
    static const unsigned int ROBE_FOOTER_SIZE = 1;
    static const unsigned int ROBE_HEADER_SIZE = 5;
};
#endif  // PLUGINS_USBPRO_MOCKENDPOINT_H_
