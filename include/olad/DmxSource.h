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
 * DmxSource.h
 * Interface for the DmxSource class.
 * A DmxSource contains a DmxSource as well as a priority & timestamp.
 * Copyright (C) 2005 Simon Newton
 */

#ifndef INCLUDE_OLAD_DMXSOURCE_H_
#define INCLUDE_OLAD_DMXSOURCE_H_

#include <stdint.h>
#include <ola/Clock.h>
#include <ola/DmxBuffer.h>
#include <ola/dmx/SourcePriorities.h>

namespace ola {


/*
 * The DmxSource class
 */
class DmxSource {
 public:
    DmxSource():
        m_buffer(),
        m_timestamp(),
        m_priority(ola::dmx::SOURCE_PRIORITY_MIN) {
    }

    DmxSource(const DmxBuffer &buffer,
              const TimeStamp &timestamp,
              uint8_t priority):
        m_buffer(buffer),
        m_timestamp(timestamp),
        m_priority(priority) {
    }

    DmxSource(const DmxSource &other) {
      m_buffer = other.m_buffer;
      m_timestamp = other.m_timestamp;
      m_priority = other.m_priority;
    }


    /*
     * Assignment operator
     */
    DmxSource& operator=(const DmxSource& other) {
      if (this != &other) {
        m_buffer = other.m_buffer;
        m_timestamp = other.m_timestamp;
        m_priority = other.m_priority;
      }
      return *this;
    }

    /*
     * Equality check
     */
    bool operator==(const DmxSource &other) const {
      return (m_buffer == other.m_buffer &&
              m_timestamp == other.m_timestamp &&
              m_priority == other.m_priority);
    }


    /*
     * Update the DmxSource with new data
     */
    void UpdateData(const DmxBuffer &buffer, const TimeStamp &timestamp,
                    uint8_t priority) {
      m_buffer = buffer;
      m_timestamp = timestamp;
      m_priority = priority;
    }


    /*
     * Get the DmxBuffer in this source
     */
    const DmxBuffer &Data() const { return m_buffer; }


    /*
     * Get the timestamp
     */
    const TimeStamp &Timestamp() const { return m_timestamp; }


    /*
     * Check if this source has timed out
     */
    bool IsActive(const TimeStamp &now) const {
      TimeStamp timeout = m_timestamp + TIMEOUT_INTERVAL;
      return now < timeout;
    }


    /*
     * Check if this source has ever got data
     */
    bool IsSet() const {
      return m_timestamp.IsSet();
    }


    /*
     * Get the priority for this source
     */
    uint8_t Priority() const { return m_priority; }

 private:
    DmxBuffer m_buffer;
    TimeStamp m_timestamp;
    uint8_t m_priority;

    static const TimeInterval TIMEOUT_INTERVAL;
};
}  // namespace ola
#endif  // INCLUDE_OLAD_DMXSOURCE_H_
