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
 * Clock.h
 * Provides the TimeInterval and TimeStamp classes.
 * Copyright (C) 2005 Simon Newton
 *
 * The struct timeval can represent both absolute time and time intervals.
 * We define our own wrapper classes that:
 *   - hide some of the platform differences, like the fact windows doesn't
 *     provide timeradd and timersub.
 *   - Reduces bugs by using the compiler to check if the value was supposed
 *     to be an interval or absolute time. For example, passing an absolute
 *     time instead of an Interval to RegisterTimeout would be bad.
 */

#ifndef INCLUDE_OLA_CLOCK_H_
#define INCLUDE_OLA_CLOCK_H_

#include <ola/base/Macro.h>
#include <stdint.h>
#include <sys/time.h>

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace ola {

static const int USEC_IN_SECONDS = 1000000;
static const int MSEC_IN_SEC = 1000;
static const uint64_t NSEC_IN_SEC = 1000000000;
static const int ONE_THOUSAND = MSEC_IN_SEC;
/**
 * Don't use this class directly. It's an implementation detail of TimeInterval
 * and TimeStamp.
 */
class BaseTimeVal {
 public:
  // Constructors
  BaseTimeVal() { timerclear(&m_tv); }
  BaseTimeVal(int32_t sec, int32_t usec);

  explicit BaseTimeVal(const struct timeval &timestamp) { m_tv = timestamp; }
  explicit BaseTimeVal(int64_t interval_useconds) { Set(interval_useconds); }

  BaseTimeVal(const BaseTimeVal &other) : m_tv(other.m_tv) {}

  // Assignable
  BaseTimeVal& operator=(const BaseTimeVal& other);
  BaseTimeVal& operator=(const struct timeval &tv);

  // Comparables
  bool operator==(const BaseTimeVal &other) const;
  bool operator!=(const BaseTimeVal &other) const;
  bool operator>(const BaseTimeVal &other) const;
  bool operator>=(const BaseTimeVal &other) const;
  bool operator<(const BaseTimeVal &other) const;
  bool operator<=(const BaseTimeVal &other) const;

  // Arithmetic
  BaseTimeVal& operator+=(const BaseTimeVal& other);
  BaseTimeVal &operator-=(const BaseTimeVal &other);
  const BaseTimeVal operator+(const BaseTimeVal &interval) const;
  const BaseTimeVal operator-(const BaseTimeVal &other) const;
  BaseTimeVal operator*(unsigned int i) const;

  // Various other methods.
  bool IsSet() const;
  void AsTimeval(struct timeval *tv) const;

  /**
   * @brief Returns the seconds portion of the BaseTimeVal
   * @return The seconds portion of the BaseTimeVal
   */
  time_t Seconds() const { return m_tv.tv_sec; }
  /**
   * @brief Returns the microseconds portion of the BaseTimeVal
   * @return The microseconds portion of the BaseTimeVal
   */
  int32_t MicroSeconds() const { return static_cast<int32_t>(m_tv.tv_usec); }

  /**
   * @brief Returns the entire BaseTimeVal as milliseconds
   * @return The entire BaseTimeVal in milliseconds
   */
  int64_t InMilliSeconds() const;

  /**
   * @brief Returns the entire BaseTimeVal as microseconds
   * @return The entire BaseTimeVal in microseconds
   */
  int64_t AsInt() const;

  /**
   * @brief InMicroSeconds returns AsInt() for naming consistency
   * @return returns AsInt()
   */
  int64_t InMicroSeconds() const;

  std::string ToString() const;

 private:
  struct timeval m_tv;

  /**
   * We don't use timeradd here because Windows doesn't define it.
   */
  void TimerAdd(const struct timeval &tv1, const struct timeval &tv2,
                struct timeval *result) const;

  /**
   * We don't use timersub here because Windows doesn't define it.
   */
  void TimerSub(const struct timeval &tv1, const struct timeval &tv2,
                struct timeval *result) const;

  void Set(int64_t interval_useconds);
};

/**
 * @brief A time interval, with usecond accuracy.
 */
class TimeInterval {
 public:
  // Constructors
  TimeInterval() {}
  TimeInterval(int32_t sec, int32_t usec) : m_interval(sec, usec) {}
  explicit TimeInterval(int64_t usec) : m_interval(usec) {}

  TimeInterval(const TimeInterval &other) : m_interval(other.m_interval) {}

  // Assignable
  TimeInterval& operator=(const TimeInterval& other);

  // Comparables
  bool operator==(const TimeInterval &other) const;
  bool operator!=(const TimeInterval &other) const;
  bool operator>(const TimeInterval &other) const;
  bool operator>=(const TimeInterval &other) const;
  bool operator<(const TimeInterval &other) const;
  bool operator<=(const TimeInterval &other) const;

  // Arithmetic
  TimeInterval& operator+=(const TimeInterval& other);
  TimeInterval operator*(unsigned int i) const;

  // Various other methods.
  bool IsZero() const { return !m_interval.IsSet(); }

  void AsTimeval(struct timeval *tv) const { m_interval.AsTimeval(tv); }

  time_t Seconds() const { return m_interval.Seconds(); }
  int32_t MicroSeconds() const { return m_interval.MicroSeconds(); }

  int64_t InMilliSeconds() const { return m_interval.InMilliSeconds(); }
  int64_t InMicroSeconds() const { return m_interval.InMicroSeconds(); }
  int64_t AsInt() const { return m_interval.AsInt(); }

  std::string ToString() const { return m_interval.ToString(); }

  friend std::ostream& operator<< (std::ostream &out,
                                   const TimeInterval &interval) {
    return out << interval.m_interval.ToString();
  }

 private:
  explicit TimeInterval(const BaseTimeVal &time_val) : m_interval(time_val) {}

  BaseTimeVal m_interval;
  friend class TimeStamp;
};


/**
 * @brief Represents a point in time with microsecond accuracy.
 */
class TimeStamp {
 public:
    // Constructors
    TimeStamp() {}
    explicit TimeStamp(const struct timeval &timestamp) : m_tv(timestamp) {}

    TimeStamp(const TimeStamp &other) : m_tv(other.m_tv) {}

    // Assignable
    TimeStamp& operator=(const TimeStamp& other);
    TimeStamp& operator=(const struct timeval &tv);

    // Comparables
    bool operator==(const TimeStamp &other) const { return m_tv == other.m_tv; }
    bool operator!=(const TimeStamp &other) const { return m_tv != other.m_tv; }
    bool operator>(const TimeStamp &other) const { return m_tv > other.m_tv; }
    bool operator>=(const TimeStamp &other) const { return m_tv >= other.m_tv; }
    bool operator<(const TimeStamp &other) const { return m_tv < other.m_tv; }
    bool operator<=(const TimeStamp &other) const { return m_tv <= other.m_tv; }

    // Arithmetic
    TimeStamp &operator+=(const TimeInterval &interval);
    TimeStamp &operator-=(const TimeInterval &interval);
    const TimeStamp operator+(const TimeInterval &interval) const;
    const TimeInterval operator-(const TimeStamp &other) const;
    const TimeStamp operator-(const TimeInterval &interval) const;

    // Various other methods.
    bool IsSet() const { return m_tv.IsSet(); }

    time_t Seconds() const { return m_tv.Seconds(); }
    int32_t MicroSeconds() const { return m_tv.MicroSeconds(); }

    std::string ToString() const { return m_tv.ToString(); }

    friend std::ostream& operator<<(std::ostream &out,
                                    const TimeStamp &timestamp) {
      return out << timestamp.m_tv.ToString();
    }

 private:
    BaseTimeVal m_tv;

    explicit TimeStamp(const BaseTimeVal &time_val) : m_tv(time_val) {}
};


/**
 * @brief Used to get the current time.
 */
class Clock {
 public:
  Clock() {}
  virtual ~Clock() {}
  virtual void CurrentTime(TimeStamp *timestamp) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(Clock);
};


/**
 * A Mock Clock used for testing.
 */
class MockClock: public Clock {
 public:
  MockClock() : Clock() {}

  // Advance the time
  void AdvanceTime(const TimeInterval &interval);
  void AdvanceTime(int32_t sec, int32_t usec);

  void CurrentTime(TimeStamp *timestamp) const;

 private:
  TimeInterval m_offset;
};

enum TimerGranularity { UNKNOWN, GOOD, BAD };

/**
 * @brief The Sleep class implements usleep with some granualtiry detection.
 */
class Sleep {
 public:
  explicit Sleep(std::string caller);

  void setCaller(std::string caller) { m_caller = caller; }

  void usleep(TimeInterval requested);
  void usleep(uint32_t requested);
  void usleep(timespec requested);

  TimerGranularity getGranularity() { return m_granularity; }
  bool CheckTimeGranularity(uint64_t wanted, uint64_t maxDeviation);
 private:
  std::string m_caller;
  uint64_t m_wanted_granularity;
  uint64_t m_max_granularity_deviation;
  uint64_t m_clock_overhead;

  static const uint32_t BAD_GRANULARITY_LIMIT = 10;

  TimerGranularity m_granularity = UNKNOWN;
};
}  // namespace ola
#endif  // INCLUDE_OLA_CLOCK_H_
