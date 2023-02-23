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
current_test=""
if [ "$1" = "spellintian" ]; then
  current_test="spellintian"
elif [ "$1" = "codespell" ]; then
  current_test="codespell"
else
  echo "Unknown test \"$1\" specified in first argument. Options are spellintian and codespell."
  exit 1;
fi;

if ! [ -x "$(command -v zrun)" ]; then
  echo "error: Cannot find zrun. Do you need the moreutils package?"
  exit 1;
fi;
if [ $current_test = "spellintian" ] && ! [ -x "$(command -v spellintian)" ]; then
  echo "error: Cannot find spellintian. Do you need the lintian package?"
  exit 1;
fi;
if [ $current_test = "codespell" ] && ! [ -x "$(command -v codespell)" ]; then
  echo "error: Cannot find codespell. Install via pip."
  exit 1;
fi;

SPELLINGBLACKLIST=$(cat <<-BLACKLIST
      -wholename "./.codespellignorelines" -or \
      -wholename "./.codespellignorewords" -or \
      -wholename "./.git/*" -or \
      -wholename "./Makefile" -or \
      -wholename "./Makefile.in" -or \
      -wholename "./aclocal.m4" -or \
      -wholename "./autom4te.cache/*" -or \
      -wholename "./common/protocol/Ola.pb.*" -or \
      -wholename "./common/rpc/TestService.pb.*" -or \
      -wholename "./config.log" -or \
      -wholename "./config.status" -or \
      -wholename "./config/config.guess" -or \
      -wholename "./config/config.sub" -or \
      -wholename "./config/depcomp" -or \
      -wholename "./config/install-sh" -or \
      -wholename "./config/libtool.m4" -or \
      -wholename "./config/ltmain.sh" -or \
      -wholename "./config/ltoptions.m4" -or \
      -wholename "./config/ltsugar.m4" -or \
      -wholename "./config/missing" -or \
      -wholename "./configure" -or \
      -wholename "./java/Makefile" -or \
      -wholename "./java/Makefile.in" -or \
      -wholename "./libtool" -or \
      -wholename "./olad/www/mobile.js" -or \
      -wholename "./olad/www/new/js/app.min.js" -or \
      -wholename "./olad/www/new/js/app.min.js.map" -or \
      -wholename "./olad/www/new/libs/angular/js/angular.min.js" -or \
      -wholename "./olad/www/new/libs/marked/js/marked.min.js" -or \
      -wholename "./olad/www/ola.js" -or \
      -wholename "./plugins/artnet/messages/ArtNetConfigMessages.pb.*" -or \
      -wholename "./tools/ola_trigger/config.tab.*" -or \
      -wholename "./tools/ola_trigger/lex.yy.cpp"
BLACKLIST
)

spellingfiles=$(eval "find ./ -type f -and ! \( \
    $SPELLINGBLACKLIST \
    \) | xargs")

if [ $current_test = "spellintian" ]; then
  # count the number of spellintian errors, including duplicate words
  # spellintian does not change the exit code, so the output must be checked
  spellintian_issues="$(zrun spellintian $spellingfiles 2>&1)"

  if [[ -n $spellintian_issues ]]; then
    printf "%s\n" "$spellintian_issues"
    # For now we always exit with success, as these errors are manually checked
    # TODO: Actively match and skip printing known false positives, exit with
    #       errors properly.
    # exit 1;
    exit 0;
  fi;
elif [ $current_test = "codespell" ]; then
  if ! zrun codespell --interactive 0 --check-filenames --check-hidden \
       --quiet 2 --regex "[a-zA-Z0-9][\\-'a-zA-Z0-9]+[a-zA-Z0-9]" \
       --exclude-file .codespellignorelines \
       --ignore-words .codespellignorewords $spellingfiles 2>&1; then
    exit 1;
  fi;
fi;
