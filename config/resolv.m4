## resolv.m4 -- Check for resolv.h
## Copyright (C) 2014 Simon Newton
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# ACX_RESOLV()
# Check that resolv.h exists and is usable.
# Sets RESOLV_LIBS
# We include netinet/in.h below to avoid
# http://lists.freebsd.org/pipermail/freebsd-bugs/2013-September/053972.html
# -----------------------------------------------------------------------------
AC_DEFUN([ACX_RESOLV],
[
AC_CHECK_HEADERS([resolv.h])
am_save_LDFLAGS="$LDFLAGS"
RESOLV_LIBS=""

acx_resolv_libs="-lresolv -resolv -lc"
for lib in $acx_resolv_libs; do
  acx_resolv_ok=no
  LDFLAGS="$am_save_LDFLAGS $lib"
  AC_MSG_CHECKING([for res_init() in $lib])
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([dnl
#include <netinet/in.h>
#include <resolv.h>], [res_init()])],
    [acx_resolv_ok=yes], [])
  AC_MSG_RESULT($acx_resolv_ok)
  if test x"$acx_resolv_ok" = xyes; then
    RESOLV_LIBS="$lib"
    break;
  fi
done
if test -z "$RESOLV_LIBS"; then
  AC_MSG_ERROR([Missing resolv, please install it])
fi
AC_SUBST(RESOLV_LIBS)

# Check for res_ninit
AC_CHECK_DECLS([res_ninit], [], [], [[
  #ifdef HAVE_SYS_TYPES_H
  # include <sys/types.h>
  #endif
  #ifdef HAVE_SYS_SOCKET_H
  # include <sys/socket.h>      /* inet_ functions / structs */
  #endif
  #ifdef HAVE_NETINET_IN_H
  # include <netinet/in.h>      /* inet_ functions / structs */
  #endif
  #ifdef HAVE_ARPA_NAMESER_H
  #  include <arpa/nameser.h> /* DNS HEADER struct */
  #endif
  #ifdef HAVE_RESOLV_H
  # include <resolv.h>
  #endif
  ]])
LDFLAGS="$am_save_LDFLAGS"
])
