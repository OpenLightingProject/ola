## maven.m4 -- macro to check for maven
## Copyright (C) 2012 Simon Newton
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

# MAVEN_SUPPORT(version)
# Check that maven is installed and is the right version
# -----------------------------------------------------------------------------
AC_DEFUN([MAVEN_SUPPORT],
[

AC_PATH_PROG([MAVEN],[mvn])
if test -z "$MAVEN" ; then
  AC_MSG_ERROR([cannot find 'mvn' program, you need to install Maven]);
elif test -n "$1" ; then
  AC_MSG_CHECKING([mvn version])
  set -x
  [maven_version=`$MAVEN --version 2> /dev/null | head -n 1 | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
  [required=$1]
  [required_major=`echo $required | sed 's/[^0-9].*//'`]
  [required_minor=`echo $required | sed 's/[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*/\1/'`]
  [required_patch=`echo $required | sed 's/^.*[^0-9]//'`]
  [actual_major=`echo $maven_version | sed 's/[^0-9].*//'`]
  [actual_minor=`echo $maven_version | sed 's/[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*/\1/'`]
  [actual_patch=`echo $maven_version | sed 's/^.*[^0-9]//'`]

  maven_version_proper=`expr \
    $actual_major \> $required_major \| \
    $actual_major \= $required_major \& \
    $actual_minor \> $required_minor \| \
    $actual_major \= $required_major \& \
    $actual_minor \= $required_minor \& \
    $actual_patch \>= $required_patch `

  if test "$maven_version_proper" = "1" ; then
    AC_MSG_RESULT([$maven_version])
  else
    AC_MSG_ERROR([mvn version too old $mavaen_version < $required]);
  fi
  set +x
fi
])
