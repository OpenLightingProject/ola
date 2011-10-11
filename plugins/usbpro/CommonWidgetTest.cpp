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
 * CommonWidgetTest.cpp
 * Common code shared amongst many of the widget test classes.
 * Copyright (C) 2011 Simon Newton
 */

#include "plugins/usbpro/CommonWidgetTest.h"

/**
 * Set up the PipeDescriptor and the MockEndpoint
 */
void CommonWidgetTest::setUp() {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  m_descriptor.Init();
  m_other_end.reset(m_descriptor.OppositeEnd());
  m_endpoint.reset(new MockEndpoint(m_other_end.get()));
  m_ss.AddReadDescriptor(&m_descriptor);
  m_ss.AddReadDescriptor(m_other_end.get());
}


/**
 * Clean up
 */
void CommonWidgetTest::tearDown() {
  m_endpoint->Verify();
  m_ss.RemoveReadDescriptor(&m_descriptor);
  m_ss.RemoveReadDescriptor(m_other_end.get());
  m_other_end.get()->Close();
  m_descriptor.Close();
}


/**
 * Pack data into a Usb Pro style frame.
 * @param label the message label
 * @param data the message data
 * @param data_size the data size
 * @param total_size, pointer which is updated with the message size.
 */
uint8_t *CommonWidgetTest::BuildUsbProMessage(uint8_t label,
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
