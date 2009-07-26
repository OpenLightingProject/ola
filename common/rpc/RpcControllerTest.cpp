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
 * RpcContollerTest.cpp
 * Test fixture for the SimpleRpcController class
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <string>
#include <cppunit/extensions/HelperMacros.h>
#include <google/protobuf/stubs/common.h>
#include "SimpleRpcController.h"

using namespace std;
using namespace ola::rpc;
using namespace google::protobuf;

class RpcControllerTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RpcControllerTest);
  CPPUNIT_TEST(testFailed);
  CPPUNIT_TEST(testCancel);
  CPPUNIT_TEST_SUITE_END();

  public:
    void testFailed();
    void testCancel();

  private:
    void Callback();
    bool m_callback_run;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RpcControllerTest);

void RpcControllerTest::testFailed() {
  SimpleRpcController controller;
  string failure = "Failed";
  controller.SetFailed(failure);
  CPPUNIT_ASSERT(controller.Failed());
  CPPUNIT_ASSERT_EQUAL(controller.ErrorText(), failure);
  controller.Reset();
  CPPUNIT_ASSERT(!controller.Failed());
}

void RpcControllerTest::Callback() {
  m_callback_run = true;
}

void RpcControllerTest::testCancel() {
  SimpleRpcController controller;
  controller.StartCancel();
  CPPUNIT_ASSERT(controller.IsCanceled());

  controller.Reset();
  CPPUNIT_ASSERT(!controller.IsCanceled());

  Closure *callback = NewCallback(this, &RpcControllerTest::Callback);
  m_callback_run = false;
  controller.NotifyOnCancel(callback);
  controller.StartCancel();
  CPPUNIT_ASSERT(m_callback_run);

  controller.Reset();
  CPPUNIT_ASSERT(!controller.IsCanceled());
  m_callback_run = false;
  controller.StartCancel();
  CPPUNIT_ASSERT(!m_callback_run);
}
