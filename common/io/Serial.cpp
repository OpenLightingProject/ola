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
 * Serial.cpp
 * Serial I/O Helper methods.
 * Copyright (C) 2014 Peter Newman
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <ola/win/CleanWindows.h>
#include <winioctl.h>
#else
#include <sys/ioctl.h>
#endif  // _WIN32
#include <unistd.h>

#include <iomanip>
#include <string>
#include <sstream>
#include <vector>

#include "ola/Logging.h"
#include "ola/base/Array.h"
#include "ola/file/Util.h"
#include "ola/io/IOUtils.h"
#include "ola/io/Serial.h"
#include "ola/StringUtils.h"

namespace ola {
namespace io {

using std::vector;
using std::string;

namespace {

string GetLockFile(const string &path) {
  const string base_name = ola::file::FilenameFromPath(path);
  return ola::file::JoinPaths(UUCP_LOCK_DIR, "LCK.." + base_name);
}

bool GetPidFromFile(const string &lock_file, pid_t *pid) {
  int fd = open(lock_file.c_str(), O_RDONLY);
  if (fd < 0) {
    if (errno == ENOENT) {
      // If it doesn't exist we're ok, any other error should be treated as if
      // a lock exists.
      *pid = 0;
      return true;
    }
    OLA_INFO << "Failed to open " << lock_file << ": " << strerror(errno);
    return false;
  }

  char buffer[100];
  int r = read(fd, buffer, arraysize(buffer));
  close(fd);
  if (r < 0) {
    OLA_INFO << "Failed to read PID from " << lock_file << ": "
             << strerror(errno);
    return false;
  }

  if (!StringToInt(string(buffer, r), pid)) {
    OLA_DEBUG << "Failed to convert contents of " << lock_file;
    return false;
  }
  return true;
}

bool ProcessExists(pid_t pid) {
#ifdef _WIN32
  // TODO(Peter): Implement this
  (void) pid;
  OLA_WARN << "Not implemented yet";
  return false;
#else
  errno = 0;
  if (0 == kill(pid, 0)) {
    return true;
  }
  return errno != ESRCH;
#endif  // _WIN32
}

bool RemoveLockFile(const string &lock_file) {
  if (unlink(lock_file.c_str())) {
    OLA_WARN << "Failed to remove UUCP lock file: " << lock_file;
    return false;
  }
  return true;
}

}  // namespace

bool UIntToSpeedT(uint32_t value, speed_t *output) {
  switch (value) {
    case BAUD_RATE_9600:
      *output = B9600;
      return true;
    case BAUD_RATE_19200:
      *output = B19200;
      return true;
    case BAUD_RATE_38400:
      *output = B38400;
      return true;
    case BAUD_RATE_57600:
      *output = B57600;
      return true;
    case BAUD_RATE_115200:
      *output = B115200;
      return true;
    case BAUD_RATE_230400:
      *output = B230400;
      return true;
  }
  return false;
}


bool AcquireUUCPLockAndOpen(const std::string &path, int oflag, int *fd) {
  // This is rather tricky since there is no real convention for LCK files.
  // If it was only a single process doing the locking we could use fnctl as
  // described in 55.6 of the Linux Programming Interface book.

  // First, check if the path exists, there's no point trying to open it if not
  if (!FileExists(path)) {
    OLA_INFO << "Device " << path << " doesn't exist, so there's no point "
                "trying to acquire a lock";
    return false;
  }

  // Second, clean up a stale lockfile.
  const string lock_file = GetLockFile(path);
  OLA_DEBUG << "Checking for " << lock_file;
  pid_t locked_pid;
  if (!GetPidFromFile(lock_file, &locked_pid)) {
    OLA_INFO << "Failed to get PID from  " << lock_file;
    return false;
  }

  if (locked_pid) {
    // This will return false even if we have the lock, this is what we want
    // since different plugins may try to open the same serial port - see issue
    // #888.
    if (ProcessExists(locked_pid)) {
      OLA_INFO << "Device " << path << " locked by PID " << locked_pid
               << " and process exists, failed to acquire lock";
      return false;
    }
    // There is a race between the read & the unlink here. I'm not convinced it
    // can be solved.
    if (!RemoveLockFile(lock_file)) {
      OLA_INFO << "Device " << path << " was locked by PID " << locked_pid
               << " which is no longer active, however failed to remove stale "
               << "lock file";
      return false;
    }
  }

  pid_t our_pid = getpid();
  // Now try to open the file in O_EXCL mode. If we fail, someone else already
  // took the lock.
  int lock_fd = open(lock_file.c_str(), O_RDWR | O_CREAT | O_EXCL,
                     S_IRUSR | S_IWUSR
#ifndef _WIN32
                     | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#endif  // !_WIN32
                     );  // NOLINT(whitespace/parens)
  if (lock_fd < 0) {
    OLA_INFO << "Failed to open " << lock_file << " in exclusive mode: "
             << strerror(errno);
    return false;
  }

  OLA_INFO << "Acquired " << lock_file;

  // write our pid to the file
  std::stringstream str;
  str << std::setw(10) << our_pid << std::endl;
  const string pid_file_contents = str.str();
  size_t r = write(lock_fd, pid_file_contents.c_str(),
                   pid_file_contents.size());
  close(lock_fd);
  if (r != pid_file_contents.size()) {
    OLA_WARN << "Failed to write complete LCK file: " << lock_file;
    RemoveLockFile(lock_file);
    return false;
  }

  // Now try to open the serial device.
  if (!TryOpen(path, oflag, fd)) {
    OLA_DEBUG << "Failed to open device " << path << " despite having the "
              << "lock file";
    RemoveLockFile(lock_file);
    return false;
  }

#if HAVE_SYS_IOCTL_H
  // As a final safety mechanism, use ioctl(TIOCEXCL) if available to prevent
  // further opens.
  if (ioctl(*fd, TIOCEXCL) == -1) {
    OLA_WARN << "TIOCEXCL " << path << " failed: " << strerror(errno);
    close(*fd);
    RemoveLockFile(lock_file);
    return false;
  }
#endif  // HAVE_SYS_IOCTL_H
  return true;
}


void ReleaseUUCPLock(const std::string &path) {
  const string lock_file = GetLockFile(path);

  pid_t locked_pid;
  if (!GetPidFromFile(lock_file, &locked_pid)) {
    return;
  }

  pid_t our_pid = getpid();
  if (our_pid == locked_pid) {
    if (RemoveLockFile(lock_file)) {
      OLA_INFO << "Released " << lock_file;
    }
  }
}
}  // namespace io
}  // namespace ola
