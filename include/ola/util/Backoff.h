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
 * Backoff.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_UTIL_BACKOFF_H_
#define INCLUDE_OLA_UTIL_BACKOFF_H_

#include <math.h>
#include <ola/Clock.h>
#include <memory>

namespace ola {

/**
 * A backoff policy will calculate how long to wait before retrying an event
 * given the number of previous failed attempts.
 */
class BackOffPolicy {
 public:
    BackOffPolicy() {}
    virtual ~BackOffPolicy() {}

    /**
     * @brief Calculate the backoff time
     * @param failed_attempts is the number of unsuccessful connection
     * attempts since the last successful connection.
     * @return how long to wait before the next attempt
     */
    virtual TimeInterval BackOffTime(unsigned int failed_attempts) const = 0;
};


/**
 * Constant time back off policy. For a duration of 1s we'd produce.
 *   1, 1, 1, 1, 1, ...
 */
class ConstantBackoffPolicy: public BackOffPolicy {
 public:
    explicit ConstantBackoffPolicy(const TimeInterval &duration)
        : m_duration(duration) {
    }

    TimeInterval BackOffTime(unsigned int) const {
      return m_duration;
    }

 private:
    const TimeInterval m_duration;
};


/**
 * A backoff policy which is:
 *   t = failed_attempts * duration
 * For a duration of 1 and a max of 5 we'd produce:
 *  0, 1, 2, 3, 4, 5, 5, 5, ...
 */
class LinearBackoffPolicy: public BackOffPolicy {
 public:
    LinearBackoffPolicy(const TimeInterval &duration, const TimeInterval &max)
        : m_duration(duration),
          m_max(max) {
    }

    TimeInterval BackOffTime(unsigned int failed_attempts) const {
      TimeInterval interval = m_duration * failed_attempts;
      if (interval > m_max)
        interval = m_max;
      return interval;
    }

 private:
    const TimeInterval m_duration;
    const TimeInterval m_max;
};


/**
 * An exponential backoff policy.
 * For an initial value of 1 and a max of 20 we'd produce:
 *  0, 1, 2, 4, 8, 16, 20, 20, ...
 */
class ExponentialBackoffPolicy: public BackOffPolicy {
 public:
    ExponentialBackoffPolicy(const TimeInterval &initial,
                             const TimeInterval &max)
        : m_initial(initial),
          m_max(max) {
    }

    TimeInterval BackOffTime(unsigned int failed_attempts) const {
      TimeInterval interval = (
          m_initial * static_cast<int>(::pow(2, failed_attempts - 1)));
      if (interval > m_max)
        interval = m_max;
      return interval;
    }

 private:
    const TimeInterval m_initial;
    const TimeInterval m_max;
};


// TODO(simon): add an ExponentialJitterBackoffPolicy

// Generates backoff times.
class BackoffGenerator {
 public:
    explicit BackoffGenerator(const BackOffPolicy *policy)
        : m_policy(policy),
          m_failures(0) {
    }

    TimeInterval Next() {
      return m_policy->BackOffTime(++m_failures);
    }

    void Reset() {
      m_failures = 0;
    }

 private:
    std::unique_ptr<const BackOffPolicy> m_policy;
    unsigned int m_failures;
};
}  // namespace ola
#endif  // INCLUDE_OLA_UTIL_BACKOFF_H_
