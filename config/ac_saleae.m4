## ac_saleae.m4 -- macros for detecting the SaleaeDevice library.
## Copyright (C) 2013 Simon Newton
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

# SALEAE_DEVICE()
# Check that the SaleaeDevice library is installed and usable. This will
# define the following:
#
# In config.h:
#   HAVE_SALEAEDEVICEAPI_H
# In Makefile.am
#   libSaleaeDevice_LIBS
#   HAVE_SALEAE_LOGIC
#
# -----------------------------------------------------------------------------
AC_DEFUN([SALEAE_DEVICE],
[
  AC_REQUIRE_CPP()
  AC_CHECK_HEADERS([SaleaeDeviceApi.h])

  libSaleaeDevice_LIBS="-lSaleaeDevice"
  AC_SUBST(libSaleaeDevice_LIBS)

  old_libs=$LIBS
  LIBS="${LIBS} ${libSaleaeDevice_LIBS}"
  AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <SaleaeDeviceApi.h>],
                       [DevicesManagerInterface::RegisterOnConnect(NULL)])],
    [have_saleae=yes],
    [have_saleae=no])
  LIBS=$old_libs
  AS_IF([test "x$have_saleae" = xno],
        [AC_MSG_WARN([SaleaeDevice library is not usable.])])

  AM_CONDITIONAL(HAVE_SALEAE_LOGIC, test "${have_saleae}" = "yes")
])
