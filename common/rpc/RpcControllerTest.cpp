/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * RpcControllerTest.cpp
 * Test fixture for the RpcController class
 * Copyright (C) 2005 Simon Newton
 */

#include <cppunit/extensions/HelperMacros.h>
#include <google/protobuf/stubs/common.h>
#include <string>

#include "common/rpc/RpcController.h"
#include "ola/testing/TestUtils.h"
#include "ola/Callback.h"

using std::string;
using ola::rpc::RpcController;
using ola::rpc::RpcController;

class RpcControllerTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RpcControllerTest);
  CPPUNIT_TEST(testFailed);
  CPPUNIT_TEST_SUITE_END();

 public:
    void testFailed();

 private:
    void Callback();
    bool m_callback_run;
};


CPPUNIT_TEST_SUITE_REGISTRATION(RpcControllerTest);

void RpcControllerTest::testFailed() {
  RpcController controller;
  string failure = "Failed";
  controller.SetFailed(failure);
  OLA_ASSERT_TRUE(controller.Failed());
  OLA_ASSERT_EQ(controller.ErrorText(), failure);
  controller.Reset();
  OLA_ASSERT_FALSE(controller.Failed());
}

void RpcControllerTest::Callback() {
  m_callback_run = true;
}
