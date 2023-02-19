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

# This script is based on Travis CI tests by Peter Newman
if ! [ -x "$(command -v zrun)" ]; then
  echo "error: Cannot find zrun. Do you need the moreutils package?"
  exit 1;
fi;
if ! [ -x "$(command -v spellintian)" ]; then
  echo "error: Cannot find spellintian. Do you need the lintian package?"
  exit 1;
fi;
if ! [ -x "$(command -v codespell)" ]; then
  echo "error: Cannot find codespell. Install via pip."
  exit 1;
fi;

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

# count the number of spellintian errors, including duplicate words
spellintian_errors="$(zrun spellintian $spellingfiles 2>&1)"
spellintian_errors_count=$([ "$spellintian_errors" == "" ] && printf "0" || (printf "%s\n" "$spellintian_errors" | wc -l))
spellintian_severity="$([ "$spellintian_errors_count" == "0" ] && printf "notice" || printf "error" )"

# count the number of codespell errors
codespell_errors="$(zrun codespell --interactive 0 --disable-colors --check-filenames --check-hidden --quiet 2 --regex "[a-zA-Z0-9][\\-'a-zA-Z0-9]+[a-zA-Z0-9]" --exclude-file .codespellignorelines --ignore-words .codespellignorewords $spellingfiles 2>&1)"
codespell_errors_count="$([ "$codespell_errors" == "" ] && printf "0" || (printf "%s\n" "$codespell_errors" | wc -l))"
codespell_severity="$([ "$codespell_errors_count" == "0" ] && printf "notice" || printf "error" )"

total_errors_count="$(($spellintian_errors_count + $codespell_errors_count))"
total_severity="$([ "$total_errors_count" == "0" ] && printf "notice" || printf "error" )"

if [[ $spellintian_errors_count -ne 0 ]]; then
  # print the output for info
  printf "%s\n" "$spellintian_errors" | sed 's/^\(.*\): \(.*\)$/error:file:\1: spellintian: \2/g'
fi;

if [[ $codespell_errors_count -ne 0 ]]; then
  # print the output for info
  printf "%s\n" "$codespell_errors" | sed 's/^\(.*\):\([0-9]\+\): \(.*\)$/error:file:\1:line \2: codespell: \3/g'
fi;

echo "$spellintian_severity: Found $spellintian_errors_count spelling errors via spellintian"
echo "$codespell_severity: Found $codespell_errors_count spelling errors via codespell"
echo "$total_severity: Found $total_errors_count total spelling errors"
if [[ $total_errors_count -ne 0 ]]; then
  exit 1;
fi;