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
 * #include <lla/Logging.h>
 *
 * // Call this once
 * lla::InitLogging(lla::LLA_LOG_WARN, lla::LLA_LOG_STDERR);
 *
 * LLA_FATAL << "foo";
 * LLA_WARN << "foo";
 * LLA_INFO << "foo";
 * LLA_DEBUG << "foo";
 */

#ifndef LLA_LOGGING_H
#define LLA_LOGGING_H

#include <ostream>
#include <string>
#include <sstream>

#define LLA_LOG(level) lla::LogLine(__FILE__, __LINE__, level).stream()
#define LLA_FATAL LLA_LOG(lla::LLA_LOG_FATAL)
#define LLA_WARN LLA_LOG(lla::LLA_LOG_WARN)
#define LLA_INFO LLA_LOG(lla::LLA_LOG_INFO)
#define LLA_DEBUG LLA_LOG(lla::LLA_LOG_DEBUG)

namespace lla {

using std::string;

/*
 * The log levels
 */
enum log_level {
  LLA_LOG_NONE,
  LLA_LOG_FATAL,
  LLA_LOG_WARN,
  LLA_LOG_INFO,
  LLA_LOG_DEBUG,
  LLA_LOG_MAX,
};

/*
 * The log outputs
 */
typedef enum {
  LLA_LOG_STDERR,
  LLA_LOG_SYSLOG,
} log_output;

/*
 * The base class for log destinations.
 */
class LogDestination {
  public:
    virtual ~LogDestination() {}
    virtual void Write(log_level level, string &log_line) = 0;
};

/*
 * A LogDestination that writes to stderr
 */
class StdErrorLogDestination: public LogDestination {
  public:
    void Write(log_level level, string &log_line);
};

/*
 * A LogDestination that writes to syslog
 */
class SyslogDestination: public LogDestination {
  public:
    void Write(log_level level, string &log_line);
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

} // lla
#endif
