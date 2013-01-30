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
 * TimeCode.h
 * Represents TimeCode
 * Copyright (C) 2011 Simon Newton
 */

#ifndef INCLUDE_OLA_TIMECODE_TIMECODE_H_
#define INCLUDE_OLA_TIMECODE_TIMECODE_H_

#include <ola/timecode/TimeCodeEnums.h>
#include <stdint.h>
#include <sstream>
#include <string>

namespace ola {
namespace timecode {


class TimeCode {
  public:
    TimeCode(TimeCodeType type, uint8_t hours, uint8_t minutes,
             uint8_t seconds, uint8_t frames)
        : m_type(type),
          m_hours(hours),
          m_minutes(minutes),
          m_seconds(seconds),
          m_frames(frames) {
    }
    TimeCode(const TimeCode &other);
    TimeCode& operator=(const TimeCode &other);
    ~TimeCode() {}

    bool IsValid() const;

    TimeCodeType Type() const { return m_type; }
    uint8_t Hours() const { return m_hours; }
    uint8_t Minutes() const { return m_minutes; }
    uint8_t Seconds() const { return m_seconds; }
    uint8_t Frames() const { return m_frames; }

    bool operator==(const TimeCode &other) const;
    bool operator!=(const TimeCode &other) const;

    std::string AsString() const;
    friend std::ostream& operator<<(std::ostream &out, const TimeCode&);

  private:
    TimeCodeType m_type;
    uint8_t m_hours;
    uint8_t m_minutes;
    uint8_t m_seconds;
    uint8_t m_frames;

    static const uint8_t MAX_HOURS = 23;
    static const uint8_t MAX_MINUTES = 59;
    static const uint8_t MAX_SECONDS = 59;
};
}  // timecode
}  // ola
#endif  // INCLUDE_OLA_TIMECODE_TIMECODE_H_
