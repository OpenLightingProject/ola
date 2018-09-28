## ola_version.m4 -- Defines for OLA Versioning
## Copyright (C) 2014 Peter Newman
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

# Define the version information
# -----------------------------------------------------------------------------
m4_define([ola_major_version], [0])
m4_define([ola_minor_version], [10])
m4_define([ola_revision_version], [7])

m4_define([ola_version],
      [ola_major_version.ola_minor_version.ola_revision_version])
