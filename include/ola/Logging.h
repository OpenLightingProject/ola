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
 * logging.h
 * Header file for the logging
 * Copyright (C) 2005-2009 Simon Newton
 *
 * How to use:
 *
 * #include <ola/Logging.h>
 *
 * // Call this once
 * ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
 *
 * OLA_FATAL << "foo";
 * OLA_WARN << "foo";
 * OLA_INFO << "foo";
 * OLA_DEBUG << "foo";
 */

#ifndef INCLUDE_OLA_LOGGING_H_
#define INCLUDE_OLA_LOGGING_H_

#include <ostream>
#include <string>
#include <sstream>

#define OLA_LOG(level) ola::LogLine(__FILE__, __LINE__, level).stream()
#define OLA_FATAL OLA_LOG(ola::OLA_LOG_FATAL)
#define OLA_WARN OLA_LOG(ola::OLA_LOG_WARN)
#define OLA_INFO OLA_LOG(ola::OLA_LOG_INFO)
#define OLA_DEBUG OLA_LOG(ola::OLA_LOG_DEBUG)

namespace ola {

using std::string;

/*
 * The log levels
 */
enum log_level {
  OLA_LOG_NONE,
  OLA_LOG_FATAL,
  OLA_LOG_WARN,
  OLA_LOG_INFO,
  OLA_LOG_DEBUG,
  OLA_LOG_MAX,
};

/*
 * The log outputs
 */
typedef enum {
  OLA_LOG_STDERR,
  OLA_LOG_SYSLOG,
  OLA_LOG_NULL,
} log_output;

/*
 * The base class for log destinations.
 */
class LogDestination {
  public:
    virtual ~LogDestination() {}
    virtual void Write(log_level level, const string &log_line) = 0;
};

/*
 * A LogDestination that writes to stderr
 */
class StdErrorLogDestination: public LogDestination {
  public:
    void Write(log_level level, const string &log_line);
};

/*
 * A LogDestination that writes to syslog
 */
class SyslogDestination: public LogDestination {
  public:
    void Write(log_level level, const string &log_line);
};

/*
 * A LogLine, this represents a single log message.
 */
class LogLine {
  public:
    LogLine(const char *file, int line, log_level level);
    ~LogLine();
    void Write();

    std::ostream &stream() { return m_stream; }
  private:
    log_level m_level;
    std::ostringstream m_stream;
    unsigned int m_prefix_length;
};

void SetLogLevel(log_level level);
void IncrementLogLevel();
void InitLogging(log_level level, log_output output);
void InitLogging(log_level level, LogDestination *destination);
}  // ola
#endif  // INCLUDE_OLA_LOGGING_H_
