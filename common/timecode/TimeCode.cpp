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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TimeCode.cpp
 * The TimeCode class
 * Copyright (C) 2011 Simon Newton
 */

#include <iomanip>
#include <iostream>
#include <string>
#include "ola/Logging.h"
#include "ola/timecode/TimeCode.h"
#include "ola/timecode/TimeCodeEnums.h"

namespace ola {
namespace timecode {

using std::setw;
using std::setfill;


TimeCode::TimeCode(const TimeCode &other)
    : m_type(other.m_type),
      m_hours(other.m_hours),
      m_minutes(other.m_minutes),
      m_seconds(other.m_seconds),
      m_frames(other.m_frames) {
}

TimeCode& TimeCode::operator=(const TimeCode &other) {
  m_type = other.m_type;
  m_hours = other.m_hours;
  m_minutes = other.m_minutes;
  m_seconds = other.m_seconds;
  m_frames = other.m_frames;
  return *this;
}

/**
 * Returns true if this timecode value is consistent, given the time
 */
bool TimeCode::IsValid() const {
  if (m_hours > MAX_HOURS ||
      m_minutes > MAX_MINUTES ||
      m_seconds > MAX_SECONDS)
    return false;

  switch (m_type) {
    case TIMECODE_FILM:
      return m_frames < 24;
    case TIMECODE_EBU:
      return m_frames < 25;
    case TIMECODE_DF:
      return m_frames < 30;
    case TIMECODE_SMPTE:
      return m_frames < 30;
  }
  return false;
}

string TimeCode::AsString() const {
  std::stringstream str;
  str << setw(2) << setfill('0') << static_cast<int>(m_hours) << ":"
    << setw(2) << setfill('0') << static_cast<int>(m_minutes) << ":"
    << setw(2) << setfill('0') << static_cast<int>(m_seconds) << ":"
    << setw(2) << setfill('0') << static_cast<int>(m_frames);
  return str.str();
}

std::ostream& operator<<(std::ostream &out, const TimeCode &t) {
  return out << t.AsString();
}

bool TimeCode::operator==(const TimeCode &other) const {
  return (m_type == other.m_type &&
          m_hours == other.m_hours &&
          m_minutes == other.m_minutes &&
          m_seconds == other.m_seconds &&
          m_frames == other.m_frames);
}


bool TimeCode::operator!=(const TimeCode &other) const {
  return !(*this == other);
}
}  // timecode
}  // ola
