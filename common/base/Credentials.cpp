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
 * Credentials.cpp
 * Handle getting and setting a process's credentials.
 * Copyright (C) 2012 Simon Newton
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>

#include <ola/Logging.h>
#include <ola/base/Credentials.h>
#include <string>

namespace ola {


/**
 * Get the effective UID
 */
uid_t GetUID() {
  return getuid();
}


/**
 * Get the effective UID
 */
uid_t GetEUID() {
  return geteuid();
}


/**
 * Get the effective GID
 */
gid_t GetGID() {
  return getgid();
}


/**
 * Get the effective GID
 */
gid_t GetEGID() {
  return getegid();
}


/**
 * Set the UID, this is a one way street.
 * Only valid if the current euid is 0, or euid == new_uid.
 */
bool SetUID(uid_t new_uid) {
  if (setuid(new_uid)) {
    OLA_WARN << "setuid failed with " << strerror(errno);
    return false;
  }
  return true;
}


/**
 * Set the UID, this is a one way street.
 * Only valid if the current egid is 0, or egid == new_gid.
 */
bool SetGID(gid_t new_gid) {
  if (setgid(new_gid)) {
    OLA_WARN << "setgid failed with " << strerror(errno);
    return false;
  }
  return true;
}


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

/**
 * Some platforms (Android) don't have the _r versions. So we fall back to the
 * non-thread safe versions.
 */
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


/**
 * Wrapper for getpwnam
 */
bool GetPasswdName(const string &name, PasswdEntry *passwd) {
#ifdef HAVE_GETPWNAM_R
  return GenericGetPasswdReentrant(getpwnam_r, name.c_str(), passwd);
#else
  return GenericGetPasswd(getpwnam, name.c_str(), passwd);
#endif
}


/**
 * Wrapper for getpwuid
 */
bool GetPasswdUID(uid_t uid, PasswdEntry *passwd) {
#ifdef HAVE_GETPWUID_R
  return GenericGetPasswdReentrant(getpwuid_r, uid, passwd);
#else
  return GenericGetPasswd(getpwuid, uid, passwd);
#endif
}


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

  if (!grp_ptr)
    return false;

  group_entry->gr_name = grp_ptr->gr_name;
  group_entry->gr_gid = grp_ptr->gr_gid;
  delete[] buffer;
  return true;
}


/**
 * Some platforms (Android) don't have the _r versions. So we fall back to the
 * non-thread safe versions.
 */
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

/**
 * Wrapper for getpwnam
 */
bool GetGroupName(const string &name, GroupEntry *group_entry) {
#ifdef HAVE_GETGRNAM_R
  return GenericGetGroupReentrant(getgrnam_r, name.c_str(), group_entry);
#else
  return GenericGetGroup(getgrnam, name.c_str(), group_entry);
#endif
}


/**
 * Wrapper for getgrgid
 */
bool GetGroupGID(gid_t uid, GroupEntry *group_entry) {
#ifdef HAVE_GETGRGID_R
  return GenericGetGroupReentrant(getgrgid_r, uid, group_entry);
#else
  return GenericGetGroup(getgrgid, uid, group_entry);
#endif
}
}  // ola
