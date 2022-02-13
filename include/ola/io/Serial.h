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
 * Serial.h
 * Serial IO functions.
 * Copyright (C) 2014 Peter Newman
 */

#ifndef INCLUDE_OLA_IO_SERIAL_H_
#define INCLUDE_OLA_IO_SERIAL_H_

#include <stdint.h>
#include <string>
#include <vector>

#ifdef _WIN32
// Define types and constants to mimic termios.h
#define B9600 9600
#define B19200 19200
#define B38400 38400
#define B57600 57600
#define B115200 115200
#define B230400 230400
typedef unsigned speed_t;
#else
#include <termios.h>
#endif  // _WIN32

namespace ola {
namespace io {

typedef enum {
  BAUD_RATE_9600 = 9600,
  BAUD_RATE_19200 = 19200,
  BAUD_RATE_38400 = 38400,
  BAUD_RATE_57600 = 57600,
  BAUD_RATE_115200 = 115200,
  BAUD_RATE_230400 = 230400,
} baud_rate;

/**
 * @brief Convert an integer baud rate to the termios struct speed_t
 * @param[in] value the baudrate value to convert
 * @param[out] output a pointer where the value will be stored
 * @returns true if the value was converted, false if the baud rate wasn't
 * supported by the method.
 */
bool UIntToSpeedT(uint32_t value, speed_t *output);

/**
 * @brief Try to open the path and obtain a lock to control access
 * @param path the path to open
 * @param oflag flags passed to open
 * @param[out] fd a pointer to the fd which is returned.
 * @returns true if the open succeeded, false otherwise.
 *
 * Depending on the compile-time configuration, this will use either flock()
 * (the default) or UUCP locking.  See: ./configure --enable-uucp-locking.
 *
 * This fails-fast, it we can't get the lock immediately, we'll return false.
 */
bool AcquireLockAndOpenSerialPort(const std::string &path, int oflag, int *fd);

/**
 * @brief Remove a UUCP lock file for the device.
 * @param path The path to unlock.
 *
 * The lock is only removed if the PID matches.
 *
 * Does nothing if UUCP locking is not in use
 * (see ./configure --enable-uucp-locking).
 */
void ReleaseUUCPLock(const std::string &path);
}  // namespace io
}  // namespace ola
#endif  // INCLUDE_OLA_IO_SERIAL_H_
