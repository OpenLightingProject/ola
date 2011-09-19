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

#include "ola/network/Socket.h"


/**
 * The MockEndpoint, this is used for the unittests.
 */
class MockEndpoint {
  public:
    explicit MockEndpoint(ola::network::ConnectedDescriptor *descriptor)
        : m_descriptor(descriptor) {
    }
    ~MockEndpoint();

    void AddExpectedData(const uint8_t *expected_data,
                         unsigned int expected_length);

    void AddExpectedDataAndReturn(const uint8_t *expected_data,
                                  unsigned int expected_length,
                                  const uint8_t *return_data,
                                  unsigned int return_length);

    void SendUnsolicited(const uint8_t *data,
                         unsigned int length);

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
      data_frame expected_data_frame;
      data_frame return_data_frame;
    } expected_data;

    std::queue<expected_data> m_expected_data;

    enum {MAX_DATA_SIZE = 600};

    void DescriptorReady();
};
#endif  // PLUGINS_USBPRO_MOCKENDPOINT_H_
