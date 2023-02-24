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
 * Clock.cpp
 * Provides the TimeInterval and TimeStamp classes.
 * Copyright (C) 2005 Simon Newton
 *
 * The struct timeval can represent both absolute time and time intervals.
 * We define our own wrapper classes that:
 *   - hide some of the platform differences, like the fact windows doesn't
 *     provide timersub.
 *   - Reduces bugs by using the compiler to check if the value was supposed
 *     to be an interval or absolute time. For example, passing an absolute
 *     time instead of an Interval to RegisterTimeout would be bad.
 */

#include <ola/Clock.h>
#include <ola/Logging.h>
#include <stdint.h>
#include <sys/time.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace ola {

using std::string;

BaseTimeVal::BaseTimeVal(int32_t sec, int32_t usec) {
  m_tv.tv_sec = sec;
  m_tv.tv_usec = usec;
}

BaseTimeVal& BaseTimeVal::operator=(const BaseTimeVal& other) {
  if (this != &other) {
    m_tv = other.m_tv;
  }
  return *this;
}

BaseTimeVal& BaseTimeVal::operator=(const struct timeval &tv) {
  m_tv = tv;
  return *this;
}

BaseTimeVal& BaseTimeVal::operator=(const struct timespec &ts) {
  Set(ts);
  return *this;
}

bool BaseTimeVal::operator==(const BaseTimeVal &other) const {
  return timercmp(&m_tv, &other.m_tv, ==);
}

bool BaseTimeVal::operator!=(const BaseTimeVal &other) const {
  return !(*this == other);
}

bool BaseTimeVal::operator>(const BaseTimeVal &other) const {
  return timercmp(&m_tv, &other.m_tv, >);
}

bool BaseTimeVal::operator>=(const BaseTimeVal &other) const {
  return timercmp(&m_tv, &other.m_tv, >=);
}

bool BaseTimeVal::operator<(const BaseTimeVal &other) const {
  return timercmp(&m_tv, &other.m_tv, <);
}

bool BaseTimeVal::operator<=(const BaseTimeVal &other) const {
  return timercmp(&m_tv, &other.m_tv, <=);
}

BaseTimeVal& BaseTimeVal::operator+=(const BaseTimeVal& other) {
  if (this != &other) {
    TimerAdd(m_tv, other.m_tv, &m_tv);
  }
  return *this;
}

BaseTimeVal &BaseTimeVal::operator-=(const BaseTimeVal &other) {
  if (this != &other) {
    TimerSub(m_tv, other.m_tv, &m_tv);
  }
  return *this;
}

const BaseTimeVal BaseTimeVal::operator+(const BaseTimeVal &interval) const {
  BaseTimeVal result = *this;
  result += interval;
  return result;
}

const BaseTimeVal BaseTimeVal::operator-(const BaseTimeVal &other) const {
  BaseTimeVal result;
  TimerSub(m_tv, other.m_tv, &result.m_tv);
  return result;
}

BaseTimeVal BaseTimeVal::operator*(unsigned int i) const {
  int64_t as_int = (*this).AsInt();
  as_int *= i;
  return BaseTimeVal(as_int);
}

bool BaseTimeVal::IsSet() const {
  return timerisset(&m_tv);
}

void BaseTimeVal::AsTimeval(struct timeval *tv) const {
  *tv = m_tv;
}

int64_t BaseTimeVal::InMilliSeconds() const {
  return (m_tv.tv_sec * static_cast<int64_t>(ONE_THOUSAND) +
          m_tv.tv_usec / ONE_THOUSAND);
}

int64_t BaseTimeVal::AsInt() const {
  return (m_tv.tv_sec * static_cast<int64_t>(USEC_IN_SECONDS) + m_tv.tv_usec);
}

string BaseTimeVal::ToString() const {
  std::ostringstream str;
  str << m_tv.tv_sec << "." << std::setfill('0') << std::setw(6)
      << m_tv.tv_usec;
  return str.str();
}

void BaseTimeVal::TimerAdd(const struct timeval &tv1, const struct timeval &tv2,
                           struct timeval *result) const {
  result->tv_sec = tv1.tv_sec + tv2.tv_sec;
  result->tv_usec = tv1.tv_usec + tv2.tv_usec;
  if (result->tv_usec >= USEC_IN_SECONDS) {
      result->tv_sec++;
      result->tv_usec -= USEC_IN_SECONDS;
  }
}

void BaseTimeVal::TimerSub(const struct timeval &tv1, const struct timeval &tv2,
                           struct timeval *result) const {
  result->tv_sec = tv1.tv_sec - tv2.tv_sec;
  result->tv_usec = tv1.tv_usec - tv2.tv_usec;
  if (result->tv_usec < 0) {
      result->tv_sec--;
      result->tv_usec += USEC_IN_SECONDS;
  }
}

void BaseTimeVal::Set(int64_t interval_useconds) {
#ifdef HAVE_TIME_T
  m_tv.tv_sec = static_cast<time_t>(
      interval_useconds / USEC_IN_SECONDS);
#else
  m_tv.tv_sec = interval_useconds / USEC_IN_SECONDS;
#endif  // HAVE_TIME_T

#ifdef HAVE_SUSECONDS_T
  m_tv.tv_usec = static_cast<suseconds_t>(interval_useconds % USEC_IN_SECONDS);
#else
    m_tv.tv_usec = interval_useconds % USEC_IN_SECONDS;
#endif  // HAVE_SUSECONDS_T
}

void BaseTimeVal::Set(const struct timespec& ts) {
  m_tv.tv_sec = ts.tv_sec;
  m_tv.tv_usec = ts.tv_nsec / ONE_THOUSAND;
}

TimeInterval& TimeInterval::operator=(const TimeInterval& other) {
  if (this != &other) {
    m_interval = other.m_interval;
  }
  return *this;
}

bool TimeInterval::operator==(const TimeInterval &other) const {
  return m_interval == other.m_interval;
}

bool TimeInterval::operator!=(const TimeInterval &other) const {
  return m_interval != other.m_interval;
}

bool TimeInterval::operator>(const TimeInterval &other) const {
  return m_interval > other.m_interval;
}

bool TimeInterval::operator>=(const TimeInterval &other) const {
  return m_interval >= other.m_interval;
}

bool TimeInterval::operator<(const TimeInterval &other) const {
  return m_interval < other.m_interval;
}

bool TimeInterval::operator<=(const TimeInterval &other) const {
  return m_interval <= other.m_interval;
}

TimeInterval& TimeInterval::operator+=(const TimeInterval& other) {
  if (this != &other) {
    m_interval += other.m_interval;
  }
  return *this;
}

TimeInterval TimeInterval::operator*(unsigned int i) const {
  return TimeInterval(m_interval * i);
}

TimeStamp& TimeStamp::operator=(const TimeStamp& other) {
  if (this != &other) {
    m_tv = other.m_tv;
  }
  return *this;
}

TimeStamp& TimeStamp::operator=(const struct timeval &tv) {
  m_tv = tv;
  return *this;
}

TimeStamp& TimeStamp::operator=(const struct timespec &ts) {
  m_tv = ts;
  return *this;
}

TimeStamp &TimeStamp::operator+=(const TimeInterval &interval) {
  m_tv += interval.m_interval;
  return *this;
}

TimeStamp &TimeStamp::operator-=(const TimeInterval &interval) {
  m_tv -= interval.m_interval;
  return *this;
}

const TimeStamp TimeStamp::operator+(const TimeInterval &interval) const {
  return TimeStamp(m_tv + interval.m_interval);
}

const TimeInterval TimeStamp::operator-(const TimeStamp &other) const {
  return TimeInterval(m_tv - other.m_tv);
}

const TimeStamp TimeStamp::operator-(const TimeInterval &interval) const {
  return TimeStamp(m_tv - interval.m_interval);
}

void Clock::CurrentMonotonicTime(TimeStamp *timestamp) const {
#ifdef CLOCK_MONOTONIC
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  *timestamp = ts;
#else
  CurrentRealTime(timestamp);
#endif
}

void Clock::CurrentRealTime(TimeStamp *timestamp) const {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *timestamp = tv;
}

void Clock::CurrentTime(TimeStamp *timestamp) const {
  CurrentRealTime(timestamp);
}

void MockClock::AdvanceTime(const TimeInterval &interval) {
  m_offset += interval;
}

void MockClock::AdvanceTime(int32_t sec, int32_t usec) {
  TimeInterval interval(sec, usec);
  m_offset += interval;
}

void MockClock::CurrentMonotonicTime(TimeStamp* timestamp) const {
#ifdef CLOCK_MONOTONIC
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  *timestamp = ts;
  *timestamp += m_offset;
#else
  OLA_DEBUG << "Monotonic clock unavailable. Falling back to CurrentRealTime.";
  CurrentRealTime(timestamp);
#endif
}

void MockClock::CurrentRealTime(TimeStamp* timestamp) const {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *timestamp = tv;
  *timestamp += m_offset;
}

void MockClock::CurrentTime(TimeStamp* timestamp) const {
  CurrentMonotonicTime(timestamp);
}
}  // namespace ola
