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
 * GenericTester.cpp
 * Run the tests.
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <iostream>
#include <string>

#include "ola/base/Env.h"
#include "ola/base/Flags.h"
#include "ola/base/Init.h"
#include "ola/Logging.h"
#include "ola/StringUtils.h"

using std::string;

#ifdef HAVE_EPOLL
DECLARE_bool(use_epoll);
#endif  // HAVE_EPOLL

#ifdef HAVE_KQUEUE
DECLARE_bool(use_kqueue);
#endif  // HAVE_KQUEUE

DECLARE_uint8(log_level);

bool GetBoolEnvVar(const string &var_name) {
  string var;
  bool result = false;
  ola::GetEnv(var_name, &var) && ola::StringToBool(var, &result);
  return result;
}

int main(int argc, char* argv[]) {
  // Default to INFO since it's tests.
  FLAGS_log_level = ola::OLA_LOG_INFO;

#ifdef HAVE_EPOLL
  FLAGS_use_epoll = GetBoolEnvVar("OLA_USE_EPOLL");
#endif  // HAVE_EPOLL

#ifdef HAVE_KQUEUE
  FLAGS_use_kqueue = GetBoolEnvVar("OLA_USE_KQUEUE");
#endif  // HAVE_KQUEUE

  ola::AppInit(&argc, argv, "[options]", "");

  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
  CppUnit::TextUi::TestRunner runner;
  runner.addTest(suite);

  runner.setOutputter(
      new CppUnit::CompilerOutputter(&runner.result(), std::cerr));
  bool wasSuccessful = runner.run();
  return wasSuccessful ? 0 : 1;
}
