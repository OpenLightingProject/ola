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
 * Logging.cpp
 * The logging functions. See include/ola/Logging.h for details on how to use
 * these.
 * Copyright (C) 2005-2009 Simon Newton
 */

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#else
#include <syslog.h>
#endif

#include <iostream>
#include <string>
#include "ola/Logging.h"

namespace ola {

using std::string;
using std::ostringstream;

LogDestination *log_target = NULL;
log_level logging_level = OLA_LOG_WARN;

/*
 * Set the log level.
 * @param level the new log level
 */
void SetLogLevel(log_level level) {
  logging_level = level;
}

/*
 * Increment the log level, We reset to OLA_LOG_FATAL when we wrap.
 */
void IncrementLogLevel() {
  logging_level = (log_level) (logging_level + 1);
  if (logging_level == OLA_LOG_MAX)
    logging_level = OLA_LOG_NONE;
}



/*
 * Init the logging system.
 * @param level the log level
 * @param output the log output
 */
bool InitLogging(log_level level, log_output output) {
  LogDestination *destination;
  if (output == OLA_LOG_SYSLOG) {
    SyslogDestination *syslog_dest = new SyslogDestination();
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


/*
 * Init the logging system.
 * @param level the log level
 * @param destination A LogDestination object
 */
void InitLogging(log_level level, LogDestination *destination) {
  SetLogLevel(level);
  if (log_target)
    delete log_target;
  log_target = destination;
}


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

void StdErrorLogDestination::Write(log_level level, const string &log_line) {
  std::cerr << log_line;
  (void) level;
}


bool SyslogDestination::Init() {
#ifdef WIN32
  m_eventlog = RegisterEventSourceA(NULL, "OLA");
  if (!m_eventlog) {
    printf("Failed to initialize event logging\n");
    return false;
  }
#endif
  return true;
}


/*
 * Write a line to the system logger. This is syslog on *nix or the event log
 * on widnows
 */
void SyslogDestination::Write(log_level level, const string &log_line) {
#ifdef WIN32
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
  ReportEventA(m_eventlog,
               pri,
               (WORD) NULL,
               (DWORD) NULL,
               NULL,
               1,
               0,
               strings,
               NULL);
#else
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
#endif
}
}  //  ola
