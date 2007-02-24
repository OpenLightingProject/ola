## readline.m4 -- provide and handle --with-readline configure option
## Copyright (C) 2000 Gary V. Vaughan
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

AC_DEFUN([PLUGIN_LIBRARY],
[AC_CHECK_LIB($1, $2,[have_$1="yes"])
AM_CONDITIONAL(HAVE_$3, test "${have_$1}" = "yes")

if test "${have_$1}" = "yes"; then
 PLUGINS="${PLUGINS} $1"
 AC_DEFINE(HAVE_$3,1, [define if lib$1 is installed])
fi
])
