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
# build_name.sh
# Copyright (C) 2023 Perry Naseck

if command -v git &> /dev/null; then
  OLA_BUILD_NAME=$(git describe --abbrev=7 --dirty --always --tags 2> /dev/null || echo 'unknown-out-of-tree')
else
  OLA_BUILD_NAME='unknown'
fi

if [ "$1" = "--debian" ]; then
  head -n 1 debian/changelog | sed -E "s/ola \((.+)-([0-9]+)\).*/\1~git-$OLA_BUILD_NAME-\2/"
else
  echo $OLA_BUILD_NAME
fi
