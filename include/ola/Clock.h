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
 * Timing.h
 * Interface for the Clock & TimeStamp classes
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLA_CLOCK_H_
#define INCLUDE_OLA_CLOCK_H_

#include <stdint.h>
#include <sys/time.h>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace ola {


using std::ostream;

static const int USEC_IN_SECONDS = 1000000;


/*
 * A time interval, with usecond accuracy
 */
class TimeInterval {
  public:
    TimeInterval() {
      Set(0);
    }

    explicit TimeInterval(int64_t interval_useconds) {
      Set(interval_useconds);
    }

    TimeInterval(const TimeInterval &other) {
      m_interval = other.m_interval;
    }

    TimeInterval(unsigned int sec, unsigned int usec) {
      m_interval.tv_sec = sec;
      m_interval.tv_usec = usec;
    }

    TimeInterval& operator=(int64_t interval_useconds) {
      Set(interval_useconds);
      return *this;
    }

    TimeInterval& operator=(const TimeInterval& other) {
      if (this != &other) {
        m_interval = other.m_interval;
      }
      return *this;
    }

    bool operator==(const TimeInterval &other) const {
      return timercmp(&m_interval, &other.m_interval, ==);
    }

    bool operator!=(const TimeInterval &other) const {
      return !(*this == other);
    }

    bool operator<(const TimeInterval &other) const {
      return timercmp(&m_interval, &other.m_interval, <);
    }

    std::string ToString() const {
      std::stringstream str;
      str << m_interval.tv_sec << "." << std::setfill('0') << std::setw(6) <<
        m_interval.tv_usec;
      return str.str();
    }

    int64_t AsInt() const {
      return (m_interval.tv_sec * static_cast<uint64_t>(USEC_IN_SECONDS) +
              m_interval.tv_usec);
    }

    time_t Seconds() const {
      return m_interval.tv_sec;
    }

    void AsTimeval(struct timeval *tv) const {
      *tv = m_interval;
    }

    friend ostream& operator<< (ostream &out, const TimeInterval &interval) {
      return out << interval.ToString();
    }

  private:
    void Set(uint64_t interval_useconds) {
      m_interval.tv_sec = static_cast<time_t>(
          interval_useconds / USEC_IN_SECONDS);
      m_interval.tv_usec = static_cast<suseconds_t>(
          interval_useconds % USEC_IN_SECONDS);    
    }
    struct timeval m_interval;

  friend class TimeStamp;
};


/*
 * Represents a point in time with usecond accuracy
 */
class TimeStamp {
  public:
    TimeStamp() {
      timerclear(&m_tv);
    }

    TimeStamp(const TimeStamp &other) {
      m_tv = other.m_tv;
    }

    explicit TimeStamp(const struct timeval &timestamp) {
      m_tv = timestamp;
    }

    TimeStamp& operator=(const TimeStamp& other) {
      if (this != &other) {
        m_tv = other.m_tv;
      }
      return *this;
    }

    TimeStamp& operator=(const struct timeval &tv) {
      m_tv = tv;
      return *this;
    }

    bool operator==(const TimeStamp &other) const {
      return timercmp(&m_tv, &other.m_tv, ==);
    }

    bool operator!=(const TimeStamp &other) const {
      return !(*this == other);
    }

    bool operator>(const TimeStamp &other) const {
      return timercmp(&m_tv, &other.m_tv, >);
    }

    bool operator<(const TimeStamp &other) const {
      return timercmp(&m_tv, &other.m_tv, <);
    }

    TimeStamp &operator+=(const TimeInterval &interval) {
      m_tv.tv_sec = m_tv.tv_sec + interval.m_interval.tv_sec;
      m_tv.tv_usec = m_tv.tv_usec + interval.m_interval.tv_usec;
      if (m_tv.tv_usec >= USEC_IN_SECONDS) {
        m_tv.tv_sec++;
        m_tv.tv_usec -= USEC_IN_SECONDS;
      }
      return *this;
    }

    TimeStamp &operator-=(const TimeInterval &interval) {
      TimerSub(m_tv, interval.m_interval, &m_tv);
      return *this;
    }

    const TimeStamp operator+(const TimeInterval &interval) const {
      TimeStamp result = *this;
      result += interval;
      return result;
    }

    const TimeInterval operator-(const TimeStamp &other) const {
      TimeInterval result;
      TimerSub(m_tv, other.m_tv, &result.m_interval);
      return result;
    }

    const TimeStamp operator-(const TimeInterval &interval) const {
      TimeStamp result;
      TimerSub(m_tv, interval.m_interval, &result.m_tv);
      return result;
    }

    bool IsSet() const {
      return timerisset(&m_tv);
    }

    std::string ToString() const {
      std::stringstream str;
      str << m_tv.tv_sec << "." << std::setfill('0') << std::setw(6) <<
        m_tv.tv_usec;
      return str.str();
    }

    friend ostream& operator<< (ostream &out, const TimeStamp &timestamp) {
      return out << timestamp.ToString();
    }

  private:
    struct timeval m_tv;

    void TimerSub(const struct timeval &tv1, const struct timeval &tv2,
                  struct timeval *result) const {
      result->tv_sec = tv1.tv_sec - tv2.tv_sec;
      result->tv_usec = tv1.tv_usec - tv2.tv_usec;
      if (result->tv_usec < 0) {
          result->tv_sec--;
          result->tv_usec += USEC_IN_SECONDS;
      }
    }
};


/*
 * Used to get the current time
 */
class Clock {
  public:
    static void CurrentTime(TimeStamp *timestamp) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      *timestamp = tv;
    }
};
}  // ola
#endif  // INCLUDE_OLA_CLOCK_H_
