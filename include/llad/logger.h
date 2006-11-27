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
 * logger.h
 * Header file for the logger class
 * Copyright (C) 2005  Simon Newton
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

class Logger {

	public:

		enum Level{EMERG,CRIT,WARN,INFO,DEBUG, LOG_MAX};
		enum Output{STDERR,SYSLOG};
		
		static Logger *instance() ;
		static Logger *instance(Logger::Level level, Logger::Output output) ;
		static void clean_up() ;
		
		void log(Logger::Level lev, const char *fmt, ...) const ;
		void increment_log_level() ;

	private :
		Logger(const Logger&);
		Logger& operator=(const Logger&);

		Logger(Logger::Level level, Logger::Output output) ;
		~Logger() ;
		
		Logger::Level m_level ;
		Logger::Output m_output ;
		static Logger *s_instance ;
		
};

#endif
