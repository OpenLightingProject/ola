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
 * Credentials.cpp
 * Handle getting and setting a process's credentials.
 * Copyright (C) 2012 Simon Newton
 */

/**
 * @addtogroup cred
 * @{
 * @file Credentials.cpp
 * @}
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif  // HAVE_CONFIG_H
 
#include <errno.h>
#ifndef _WIN32
#include <grp.h>
#include <pwd.h>
#endif  // _WIN32
#include <string.h>
#include <unistd.h>

#include <ola/Logging.h>
#include <ola/base/Credentials.h>
#include <string>

namespace ola {

using std::string;

/**
 * @addtogroup cred
 * @{
 */

 bool SupportsUIDs() {
#ifdef _WIN32
  return false;
#else
  return true;
#endif  // _WIN32
}

bool GetUID(uid_t* uid)
{
#ifdef _WIN32
  (void) uid;
  return false;
#else
  if (uid) {
    *uid = getuid();
    return true;
  } else 
    return false;
  
#endif  // _WIN32
}


bool GetEUID(uid_t* euid) {
#ifdef _WIN32
  (void) euid;
  return false;
#else
  if (euid) {
    *euid = geteuid();
    return true;
  } else {
    return false;
  }
#endif  // _WIN32
}


bool GetGID(gid_t* gid) {
#ifdef _WIN32
  (void) gid;
  return false;
#else
  if (gid) {
    *gid = getgid();
    return true;
  } else {
    return false;
  }
#endif  // _WIN32
}


bool GetEGID(gid_t* egid) {
#ifdef _WIN32
  (void) egid;
  return false;
#else
  if (egid) {
    *egid = getegid();
    return true;
  } else {
    return false;
  }
#endif  // _WIN32
}


bool SetUID(uid_t new_uid) {
#ifdef _WIN32
  (void) new_uid;
  return false;
#else
  if (setuid(new_uid)) {
    OLA_WARN << "setuid(" << new_uid << "): " << strerror(errno);
    return false;
  }
  return true;
#endif  // _WIN32
}


bool SetGID(gid_t new_gid) {
#ifdef _WIN32
  (void) new_gid;
  return false;
#else
  if (setgid(new_gid)) {
    OLA_WARN << "setgid(" << new_gid << "): " << strerror(errno);
    return false;
  }
  return true;
#endif  // _WIN32
}

int GetGroups(int size, gid_t list[]) {
#ifdef _WIN32
  (void) size;
  (void) list;
  return -1;
#else
  return getgroups(size, list);
#endif  // _WIN32
}

bool SetGroups(size_t size, const gid_t *list) {
#ifdef _WIN32
  (void) size;
  (void) list;
  return false;
#else
  if (setgroups(size, list)) {
    OLA_WARN << "setgroups(): " << strerror(errno);
    return false;
  }
  return true;
#endif  // _WIN32
}

/**
 * @}
 */

#ifndef _WIN32

/** @private */
template <typename F, typename arg>
bool GenericGetPasswdReentrant(F f, arg a, PasswdEntry *passwd) {
  if (!passwd)
    return false;

  struct passwd pwd, *pwd_ptr;
  unsigned int size = 1024;
  bool ok = false;
  char *buffer;

  while (!ok) {
    buffer = new char[size];
    int ret = f(a, &pwd, buffer, size, &pwd_ptr);
    switch (ret) {
      case 0:
        ok = true;
        break;
      case ERANGE:
        delete[] buffer;
        size += 1024;
        break;
      default:
        delete[] buffer;
        return false;
    }
  }

  if (!pwd_ptr)
    return false;

  passwd->pw_name = pwd_ptr->pw_name;
  passwd->pw_uid = pwd_ptr->pw_uid;
  passwd->pw_gid = pwd_ptr->pw_gid;
  passwd->pw_dir = pwd_ptr->pw_dir;
  passwd->pw_shell = pwd_ptr->pw_shell;
  delete[] buffer;
  return true;
}

/*
 * Some platforms (Android) don't have the _r versions. So we fall back to the
 * non-thread safe versions.
 */
/** @private */
template <typename F, typename arg>
bool GenericGetPasswd(F f, arg a, PasswdEntry *passwd) {
  if (!passwd)
    return false;

  struct passwd *pwd = f(a);
  if (!pwd)
    return false;

  passwd->pw_name = pwd->pw_name;
  passwd->pw_uid = pwd->pw_uid;
  passwd->pw_gid = pwd->pw_gid;
  passwd->pw_dir = pwd->pw_dir;
  passwd->pw_shell = pwd->pw_shell;
  return true;
}

#endif  // !_WIN32


bool GetPasswdName(const string &name, PasswdEntry *passwd) {
#ifdef _WIN32
  (void) name;
  (void) passwd;
  return false;
#else
#ifdef HAVE_GETPWNAM_R
  return GenericGetPasswdReentrant(getpwnam_r, name.c_str(), passwd);
#else
  return GenericGetPasswd(getpwnam, name.c_str(), passwd);
#endif  // HAVE_GETPWNAM_R
#endif  // _WIN32
}


bool GetPasswdUID(uid_t uid, PasswdEntry *passwd) {
#ifdef _WIN32
  (void) uid;
  (void) passwd;
  return false;
#else
#ifdef HAVE_GETPWUID_R
  return GenericGetPasswdReentrant(getpwuid_r, uid, passwd);
#else
  return GenericGetPasswd(getpwuid, uid, passwd);
#endif  // HAVE_GETPWUID_R
#endif  // _WIN32
}

#ifndef _WIN32

/** @private */
template <typename F, typename arg>
bool GenericGetGroupReentrant(F f, arg a, GroupEntry *group_entry) {
  if (!group_entry)
    return false;

  struct group grp, *grp_ptr;
  unsigned int size = 1024;
  bool ok = false;
  char *buffer;

  while (!ok) {
    buffer = new char[size];
    int ret = f(a, &grp, buffer, size, &grp_ptr);
    switch (ret) {
      case 0:
        ok = true;
        break;
      case ERANGE:
        delete[] buffer;
        size += 1024;
        break;
      default:
        delete[] buffer;
        return false;
    }
  }

  if (!grp_ptr) {
    // not found
    return false;
  }

  group_entry->gr_name = grp_ptr->gr_name;
  group_entry->gr_gid = grp_ptr->gr_gid;
  delete[] buffer;
  return true;
}


/*
 * Some platforms (Android) don't have the _r versions. So we fall back to the
 * non-thread safe versions.
 */
/** @private */
template <typename F, typename arg>
bool GenericGetGroup(F f, arg a, GroupEntry *group_entry) {
  if (!group_entry)
    return false;

  struct group *grp_ptr = f(a);
  if (!grp_ptr)
    return false;

  group_entry->gr_name = grp_ptr->gr_name;
  group_entry->gr_gid = grp_ptr->gr_gid;
  return true;
}

#endif  // !_WIN32


bool GetGroupName(const string &name, GroupEntry *group_entry) {
#ifdef _WIN32
  (void) name;
  (void) group_entry;
  return false;
#else
#ifdef HAVE_GETGRNAM_R
  return GenericGetGroupReentrant(getgrnam_r, name.c_str(), group_entry);
#else
  return GenericGetGroup(getgrnam, name.c_str(), group_entry);
#endif  // HAVE_GETGRNAM_R
#endif  // _WIN32
}


bool GetGroupGID(gid_t gid, GroupEntry *group_entry) {
#ifdef _WIN32
  (void) gid;
  (void) group_entry;
  return false;
#else
#ifdef HAVE_GETGRGID_R
  return GenericGetGroupReentrant(getgrgid_r, gid, group_entry);
#else
  return GenericGetGroup(getgrgid, gid, group_entry);
#endif  // HAVE_GETGRGID_R
#endif  // _WIN32
}
}  // namespace ola
