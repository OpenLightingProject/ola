/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * CommonWidgetTest.h
 * Common code shared amongst many of the widget test classes.
 * Copyright (C) 2011 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "ola/Logging.h"
#include "ola/io/Descriptor.h"
#include "ola/io/SelectServer.h"
#include "plugins/usbpro/MockEndpoint.h"


using std::auto_ptr;


#ifndef PLUGINS_USBPRO_COMMONWIDGETTEST_H_
#define PLUGINS_USBPRO_COMMONWIDGETTEST_H_

class CommonWidgetTest: public CppUnit::TestFixture {
  public:
    virtual void setUp();
    virtual void tearDown();

  protected:
    ola::io::SelectServer m_ss;
    ola::io::PipeDescriptor m_descriptor;
    auto_ptr<ola::io::PipeDescriptor> m_other_end;
    auto_ptr<MockEndpoint> m_endpoint;

    void Terminate() { m_ss.Terminate(); }

    uint8_t *BuildUsbProMessage(uint8_t label,
                                const uint8_t *data,
                                unsigned int data_size,
                                unsigned int *total_size);

    static const unsigned int FOOTER_SIZE = 1;
    static const unsigned int HEADER_SIZE = 4;
};
#endif  // PLUGINS_USBPRO_COMMONWIDGETTEST_H_
