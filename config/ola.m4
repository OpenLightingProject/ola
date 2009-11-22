## ola.m4 -- macros for OLA
## Copyright (C) 2009 Simon Newton
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

# PLUGIN_LIBRARY(library, function, plugin_name)
# Check for function in library and set HAVE_${plugin_name}
# -----------------------------------------------------------------------------
AC_DEFUN([PLUGIN_LIBRARY],
[AC_CHECK_LIB($1, $2,[have_$1="yes"])
AM_CONDITIONAL(HAVE_$3, test "${have_$1}" = "yes")

if test "${have_$1}" = "yes"; then
 PLUGINS="${PLUGINS} $1"
 AC_DEFINE(HAVE_$3,1, [define if lib$1 is installed])
fi
])

# PROTOBUF_SUPPORT(version)
# Check that the protobuf headers are installed and that protoc is the correct
# version
# -----------------------------------------------------------------------------
AC_DEFUN([PROTOBUF_SUPPORT],
[
AC_REQUIRE_CPP()
AC_CHECK_HEADER(google/protobuf/stubs/common.h, ,
                AC_MSG_ERROR('protobuf library not installed'))
AC_PATH_PROG([PROTOC],[protoc])
if test -z "$PROTOC" ; then
  AC_MSG_ERROR([cannot find 'protoc' program]);
elif test -n "$1" ; then
  AC_MSG_CHECKING([protoc version])
  [protoc_version=`$PROTOC --version 2>&1 | grep 'libprotoc' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
  [required=$1]
  [required_major=`echo $required | sed 's/[^0-9].*//'`]
  [required_minor=`echo $required | sed 's/[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*/\1/'`]
  [required_patch=`echo $required | sed 's/^.*[^0-9]//'`]
  [actual_major=`echo $protoc_version | sed 's/[^0-9].*//'`]
  [actual_minor=`echo $protoc_version | sed 's/[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*/\1/'`]
  [actual_patch=`echo $protoc_version | sed 's/^.*[^0-9]//'`]

  protoc_version_proper=`expr \
    $actual_major \> $required_major \| \
    $actual_major \= $required_major \& \
    $actual_minor \> $required_minor \| \
    $actual_major \= $required_major \& \
    $actual_minor \= $required_minor \& \
    $actual_patch \>= $required_patch `

  if test "$protoc_version_proper" = "1" ; then
    AC_MSG_RESULT([$protoc_version])
  else
    AC_MSG_ERROR([protoc version too old $protoc_version < $required]);
  fi
fi
])
