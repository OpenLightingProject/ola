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
 * Logging.cpp
 * The logging functions. See include/ola/Logging.h for details on how to use
 * these.
 * Copyright (C) 2005 Simon Newton
 */

/**
 * @addtogroup logging
 * @{
 *
 * @file Logging.cpp
 *
 * @}
 */
#include <stdio.h>

#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <ola/win/CleanWindows.h>
#include <io.h>
#else
#include <syslog.h>
#endif  // _WIN32

#include <unistd.h>

#include <iostream>
#include <string>
#include "ola/Logging.h"
#include "ola/base/Flags.h"

/**@private*/
DEFINE_s_int8(log_level, l, ola::OLA_LOG_WARN, "Set the logging level 0 .. 4.");
/**@private*/
DEFINE_default_bool(syslog, false, "Send to syslog rather than stderr.");

namespace ola {

using std::ostringstream;
using std::string;

/**
 * @cond HIDDEN_SYMBOLS
 * @brief pointer to a log target
 */
LogDestination *log_target = NULL;

log_level logging_level = OLA_LOG_WARN;
/**@endcond*/

/**
 * @addtogroup logging
 * @{
 */
void SetLogLevel(log_level level) {
  logging_level = level;
}


void IncrementLogLevel() {
  logging_level = (log_level) (logging_level + 1);
  if (logging_level == OLA_LOG_MAX)
    logging_level = OLA_LOG_NONE;
}


bool InitLoggingFromFlags() {
  log_output output = OLA_LOG_NULL;
  if (FLAGS_syslog) {
    output = OLA_LOG_SYSLOG;
  } else {
    output = OLA_LOG_STDERR;
  }

  log_level log_level = ola::OLA_LOG_WARN;
  switch (FLAGS_log_level) {
    case 0:
      // nothing is written at this level
      // so this turns logging off
      log_level = ola::OLA_LOG_NONE;
      break;
    case 1:
      log_level = ola::OLA_LOG_FATAL;
      break;
    case 2:
      log_level = ola::OLA_LOG_WARN;
      break;
    case 3:
      log_level = ola::OLA_LOG_INFO;
      break;
    case 4:
      log_level = ola::OLA_LOG_DEBUG;
      break;
    default :
      break;
  }

  return InitLogging(log_level, output);
}


bool InitLogging(log_level level, log_output output) {
  LogDestination *destination;
  if (output == OLA_LOG_SYSLOG) {
#ifdef _WIN32
    SyslogDestination *syslog_dest = new WindowsSyslogDestination();
#else
    SyslogDestination *syslog_dest = new UnixSyslogDestination();
#endif  // _WIN32
    if (!syslog_dest->Init()) {
      delete syslog_dest;
      return false;
    }
    destination = syslog_dest;
  } else if (output == OLA_LOG_STDERR) {
    destination = new StdErrorLogDestination();
  } else {
    destination = NULL;
  }
  InitLogging(level, destination);
  return true;
}


void InitLogging(log_level level, LogDestination *destination) {
  SetLogLevel(level);
  if (log_target) {
    delete log_target;
  }
  log_target = destination;
}

/**@}*/
/**@cond HIDDEN_SYMBOLS*/
LogLine::LogLine(const char *file,
                 int line,
                 log_level level):
  m_level(level),
  m_stream(ostringstream::out) {
    m_stream << file << ":" << line << ": ";
    m_prefix_length = m_stream.str().length();
}

LogLine::~LogLine() {
  Write();
}

void LogLine::Write() {
  if (m_stream.str().length() == m_prefix_length)
    return;

  if (m_level > logging_level)
    return;

  string line = m_stream.str();

  if (line.at(line.length() - 1) != '\n')
    line.append("\n");

  if (log_target)
    log_target->Write(m_level, line);
}
/**@endcond*/

/**
 * @addtogroup logging
 * @{
 */
void StdErrorLogDestination::Write(log_level level, const string &log_line) {
  ssize_t bytes = write(STDERR_FILENO, log_line.c_str(), log_line.size());
  (void) bytes;
  (void) level;
}

#ifdef _WIN32
bool WindowsSyslogDestination::Init() {
  m_eventlog = RegisterEventSourceA(NULL, "OLA");
  if (!m_eventlog) {
    printf("Failed to initialize event logging\n");
    return false;
  }
  return true;
}

void WindowsSyslogDestination::Write(log_level level, const string &log_line) {
  WORD pri;
  const char* strings[1];
  strings[0] = log_line.data();

  switch (level) {
    case OLA_LOG_FATAL:
      pri = EVENTLOG_ERROR_TYPE;
      break;
    case OLA_LOG_WARN:
      pri = EVENTLOG_WARNING_TYPE;
      break;
    case OLA_LOG_INFO:
      pri = EVENTLOG_INFORMATION_TYPE;
      break;
    case OLA_LOG_DEBUG:
      pri = EVENTLOG_INFORMATION_TYPE;
      break;
    default:
      pri = EVENTLOG_INFORMATION_TYPE;
  }
  ReportEventA(reinterpret_cast<HANDLE>(m_eventlog),
               pri,
               (WORD) NULL,
               (DWORD) NULL,
               NULL,
               1,
               0,
               strings,
               NULL);
}
#else
bool UnixSyslogDestination::Init() {
  return true;
}

void UnixSyslogDestination::Write(log_level level, const string &log_line) {
  int pri;
  switch (level) {
    case OLA_LOG_FATAL:
      pri = LOG_CRIT;
      break;
    case OLA_LOG_WARN:
      pri = LOG_WARNING;
      break;
    case OLA_LOG_INFO:
      pri = LOG_INFO;
      break;
    case OLA_LOG_DEBUG:
      pri = LOG_DEBUG;
      break;
    default :
      pri = LOG_INFO;
  }
  syslog(pri, "%s", log_line.data());
}
#endif  // _WIN32

}  // namespace  ola
/**@}*/
