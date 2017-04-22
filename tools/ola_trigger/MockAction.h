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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * MockAction.h
 * A mock action uses for testing.
 * Copyright (C) 2011 Simon Newton
 */

#ifndef TOOLS_OLA_TRIGGER_MOCKACTION_H_
#define TOOLS_OLA_TRIGGER_MOCKACTION_H_

#include <cppunit/extensions/HelperMacros.h>
#include <ola/testing/TestUtils.h>
#include <queue>
#include <sstream>

#include "tools/ola_trigger/Action.h"

/**
 * @brief The MockAction we use for testing.
 */
class MockAction: public Action {
 public:
  MockAction() : Action() {}

  void Execute(Context*, uint8_t slot_value) {
    m_values.push(slot_value);
  }

  void CheckForValue(const ola::testing::SourceLine &source_line,
                     uint8_t expected_value) {
    ola::testing::_AssertEquals(source_line,
                                static_cast<size_t>(1),
                                m_values.size(),
                                "Queue sizes differ");

    uint8_t value = m_values.front();
    ola::testing::_AssertEquals(source_line,
                                static_cast<int>(expected_value),
                                static_cast<int>(value),
                                "Values differ");
    m_values.pop();
  }

  bool NoCalls() const { return m_values.empty(); }

 private:
  std::queue<uint8_t> m_values;
};


/**
 * @brief An action that should never be run.
 */
class BadAction: public Action {
 public:
  BadAction() : Action() {}

  void Execute(Context*, uint8_t slot_value) {
    std::ostringstream str;
    str << "Incorrect action called for " << static_cast<int>(slot_value);
    CPPUNIT_FAIL(str.str());
  }
};
#endif  // TOOLS_OLA_TRIGGER_MOCKACTION_H_
