#!/usr/bin/env bash
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# count_generic_nolints.sh
# Copyright (C) 2023 Perry Naseck, Peter Newman

# This script is based on a Travis CI test by Peter Newman
nolints="$(grep -n --exclude "$(basename $0)" -IR NOLINT * | grep -v "NOLINT(" | sed 's/^\(.*\):\([0-9]\+\):\(.*\)$/error:file:\.\/\1:line \2: Generic NOLINT not permitted/g')"
nolints_count="$(grep --exclude "$(basename $0)" -IR NOLINT * | grep -c -v "NOLINT(")"
if [[ $nolints_count -ne 0 ]]; then
  # print the output for info
  printf "%s\n" "$nolints"
  printf "error: Found $nolints_count generic NOLINTs\n"
  exit 1
else
  printf "notice: Found $nolints_count generic NOLINTs\n"
fi
