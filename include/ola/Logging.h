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
 * Logging.h
 * Header file for the logging
 * Copyright (C) 2005-2009 Simon Newton
 */
/**
 * @file
 * @brief The OLA logging system.
 *
 * ~~~~~~~~~~~~~~~~~~~~~
 * #include <ola/Logging.h>
 *
 * // Call this once
 * ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);
 *
 * OLA_FATAL << "foo";
 * OLA_WARN << "foo";
 * OLA_INFO << "foo";
 * OLA_DEBUG << "foo";
 * ~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef INCLUDE_OLA_LOGGING_H_
#define INCLUDE_OLA_LOGGING_H_

#ifdef WIN32
#include <windows.h>  // for HANDLE
#endif

#include <ostream>
#include <string>
#include <sstream>

#define OLA_LOG(level) (level <= ola::LogLevel()) && \
                       ola::LogLine(__FILE__, __LINE__, level).stream()
#define OLA_FATAL OLA_LOG(ola::OLA_LOG_FATAL)
#define OLA_WARN OLA_LOG(ola::OLA_LOG_WARN)
#define OLA_INFO OLA_LOG(ola::OLA_LOG_INFO)
#define OLA_DEBUG OLA_LOG(ola::OLA_LOG_DEBUG)

namespace ola {

using std::string;

/**
 * @brief The OLA log levels.
 * This controls the verbosity of logging. Each level also includes those below
 * it.
 */
enum log_level {
  OLA_LOG_NONE,   ///< No messages are logged.
  OLA_LOG_FATAL,  ///< Fatal messages are logged.
  OLA_LOG_WARN,   ///< Warnings messages are logged.
  OLA_LOG_INFO,   ///< Informational messages are logged.
  OLA_LOG_DEBUG,  ///< Debug messages are logged.
  OLA_LOG_MAX,
};

extern log_level logging_level;

/**
 * @brief The destination to write log messages to
 */
typedef enum {
  OLA_LOG_STDERR,  ///< Log to stderr
  OLA_LOG_SYSLOG,  ///< Log to syslog
  OLA_LOG_NULL,
} log_output;

/**
 * The base class for log destinations.
 */
class LogDestination {
  public:
    virtual ~LogDestination() {}
    virtual void Write(log_level level, const string &log_line) = 0;
};

/**
 * A LogDestination that writes to stderr
 */
class StdErrorLogDestination: public LogDestination {
  public:
    void Write(log_level level, const string &log_line);
};

/**
 * A LogDestination that writes to syslog
 */
class SyslogDestination: public LogDestination {
  public:
    bool Init();
    void Write(log_level level, const string &log_line);
  private:
#ifdef WIN32
    HANDLE m_eventlog;
#endif
};

/**
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

/**
 * Set the logging level
 */
void SetLogLevel(log_level level);
inline log_level LogLevel() { return logging_level; }

/**
 * Increment the log level by one. The log level wraps to OLA_LOG_NONE.
 */
void IncrementLogLevel();

/**
 * Initialize the OLA logging system from flags. ParseFlags must have been
 * called before calling this.
 */
bool InitLoggingFromFlags();

/**
 * Initialize the OLA logging system
 * @param level the level to log at
 * @param output the destintion for the logs
 * @returns true if logging was initialized sucessfully, false otherwise.
 */
bool InitLogging(log_level level, log_output output);
void InitLogging(log_level level, LogDestination *destination);
}  // namespace ola
#endif  // INCLUDE_OLA_LOGGING_H_
