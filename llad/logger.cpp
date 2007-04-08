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
 * logger.cpp
 * Implementation of the logger class
 * Copyright (C) 2005 Simon Newton
 */

#include <llad/logger.h>
#include <stdio.h>
#include <syslog.h>

Logger *Logger::s_instance = NULL;

Logger::Level operator++(Logger::Level &l) {
  return l = static_cast<Logger::Level>(l+1);

}
/*
 * create a new logger
 *
 * @param level    the level to log at
 * @param output  where to send the logs to
 *
 */
Logger::Logger(Logger::Level level, Logger::Output output) {
  m_level = level;
  m_output = output;

  if(m_output == Logger::SYSLOG) {
    openlog("llad", 0, LOG_USER);
  }
}

/*
 * Destroy this logger object
 */
Logger::~Logger() {
  if(m_output == Logger::SYSLOG) {
    closelog();
  }
}


/*
 * Log function
 *
 * @param level  level this msg is at
 * @param fmt
 * @param ap
 */
void Logger::log(Logger::Level level, const char *fmt, ...) const {
  int pri;
  va_list ap;
  va_start(ap, fmt);

  if(level <= m_level) {

    if(m_output == Logger::SYSLOG) {
      switch (m_level) {
        case Logger::EMERG:
          pri = LOG_EMERG;
          break;
        case Logger::CRIT:
          pri = LOG_CRIT;
          break;
        case Logger::WARN:
          pri = LOG_WARNING;
          break;
        case Logger::INFO:
          pri = LOG_INFO;
          break;
        case Logger::DEBUG:
          pri = LOG_DEBUG;
          break;
        default :
          pri = LOG_INFO;
      }
      vsyslog(pri, fmt, ap);
    } else {
      vprintf(fmt, ap);
      printf("\n");
    }
  }
  va_end(ap);
}

void Logger::increment_log_level() {
  ++m_level;

  if(m_level == LOG_MAX) {
    m_level = EMERG;
  }

  log(EMERG, "Changed log level to %i\n", m_level);

}

/*
 * Grab an instance of the logger
 *
 * @return the logger object
 */
Logger *Logger::instance() {

  if(Logger::s_instance == NULL) {
    Logger::s_instance = new Logger(Logger::CRIT, Logger::STDERR);
  }
  return Logger::s_instance;
}


/*
 * Grab an instance of the logger, setting the level and output
 * if it doesn't already exist
 *
 * @param level    the log level
 * @param output  where to send the logs to
 *
 * @return the logger object
 */
Logger *Logger::instance(Logger::Level level, Logger::Output output) {

  if(Logger::s_instance == NULL) {
    Logger::s_instance = new Logger(level, output);
  }
  return Logger::s_instance;
}


/*
 * Delete the logger instance
 *
 */
void Logger::clean_up() {
  if(Logger::s_instance != NULL) {
    delete Logger::s_instance;
    Logger::s_instance = NULL;
  }
}



