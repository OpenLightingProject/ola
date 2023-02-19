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
# spellintian.sh
# Copyright (C) 2023 Perry Naseck, Peter Newman

# This script is based on a Travis CI test by Peter Newman
SPELLINGBLACKLIST=$(cat <<-BLACKLIST
      -wholename "./.codespellignorewords" -or \
      -wholename "./.codespellignorelines" -or \
      -wholename "./.git/*" -or \
      -wholename "./aclocal.m4" -or \
      -wholename "./config/config.guess" -or \
      -wholename "./config/config.sub" -or \
      -wholename "./config/depcomp" -or \
      -wholename "./config/install-sh" -or \
      -wholename "./config/libtool.m4" -or \
      -wholename "./config/ltmain.sh" -or \
      -wholename "./config/ltoptions.m4" -or \
      -wholename "./config/ltsugar.m4" -or \
      -wholename "./config/missing" -or \
      -wholename "./libtool" -or \
      -wholename "./config.log" -or \
      -wholename "./config.status" -or \
      -wholename "./Makefile" -or \
      -wholename "./Makefile.in" -or \
      -wholename "./autom4te.cache/*" -or \
      -wholename "./java/Makefile" -or \
      -wholename "./java/Makefile.in" -or \
      -wholename "./olad/www/new/js/app.min.js" -or \
      -wholename "./olad/www/new/js/app.min.js.map" -or \
      -wholename "./olad/www/new/libs/angular/js/angular.min.js" -or \
      -wholename "./olad/www/new/libs/marked/js/marked.min.js" -or \
      -wholename "./olad/www/mobile.js" -or \
      -wholename "./olad/www/ola.js" -or \
      -wholename "./configure" -or \
      -wholename "./common/protocol/Ola.pb.*" -or \
      -wholename "./plugins/artnet/messages/ArtNetConfigMessages.pb.*" -or \
      -wholename "./tools/ola_trigger/config.tab.*" -or \
      -wholename "./tools/ola_trigger/lex.yy.cpp"
BLACKLIST
)

spellingfiles=$(eval "find ./ -type f -and ! \( \
    $SPELLINGBLACKLIST \
    \) | xargs")
# count the number of spellintian errors, ignoring duplicate words
spellingerrors="$(zrun spellintian $spellingfiles 2>&1)"
spellingerrors_count=$([ "$spellingerrors" == "" ] && printf "0" || (printf "%s\n" "$spellingerrors" | wc -l))
if [[ $spellingerrors_count -ne 0 ]]; then
  # print the output for info
  printf "%s\n" "$spellingerrors"| sed 's/^\(.*\): \(.*\)$/error:file:\1: \2/g'
  echo "error: Found $spellingerrors_count spelling errors via spellintian"
  exit 1;
else
  echo "notice: Found $spellingerrors_count spelling errors via spellintian"
fi;
