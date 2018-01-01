#!/bin/bash

# This script is triggered from the script section of .travis.yml
# It runs the appropriate commands depending on the task requested.

set -e

CPP_LINT_URL="https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py";

COVERITY_SCAN_BUILD_URL="https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh"

SPELLINGBLACKLIST=$(cat <<-BLACKLIST
      -wholename "./.git/*" -or \
      -wholename "./aclocal.m4" -or \
      -wholename "./config/depcomp" -or \
      -wholename "./config/ltmain.sh" -or \
      -wholename "./config/config.guess" -or \
      -wholename "./config/install-sh" -or \
      -wholename "./config/libtool.m4" -or \
      -wholename "./config/ltoptions.m4" -or \
      -wholename "./libtool" -or \
      -wholename "./config.status" -or \
      -wholename "./Makefile" -or \
      -wholename "./Makefile.in" -or \
      -wholename "./autom4te.cache/*" -or \
      -wholename "./java/Makefile" -or \
      -wholename "./java/Makefile.in" -or \
      -wholename "./configure" -or \
      -wholename "./tools/ola_trigger/config.tab.*" -or \
      -wholename "./tools/ola_trigger/lex.yy.cpp"
BLACKLIST
)

if [[ $TASK = 'lint' ]]; then
  # run the lint tool only if it is the requested task
  autoreconf -i;
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for linting to run against
  make builtfiles
  # first check we've not got any generic NOLINTs
  # count the number of generic NOLINTs
  nolints=$(grep -IR NOLINT * | grep -v "NOLINT(" | wc -l)
  if [[ $nolints -ne 0 ]]; then
    # print the output for info
    echo $(grep -IR NOLINT * | grep -v "NOLINT(")
    echo "Found $nolints generic NOLINTs"
    exit 1;
  else
    echo "Found $nolints generic NOLINTs"
  fi;
  # then fetch and run the main cpplint tool
  wget -O cpplint.py $CPP_LINT_URL;
  chmod u+x cpplint.py;
  ./cpplint.py \
    --filter=-legal/copyright,-readability/streams,-runtime/arrays \
    $(find ./ \( -name "*.h" -or -name "*.cpp" \) -and ! \( \
        -wholename "./common/protocol/Ola.pb.*" -or \
        -wholename "./common/rpc/Rpc.pb.*" -or \
        -wholename "./common/rpc/TestService.pb.*" -or \
        -wholename "./common/rdm/Pids.pb.*" -or \
        -wholename "./config.h" -or \
        -wholename "./plugins/*/messages/*ConfigMessages.pb.*" -or \
        -wholename "./tools/ola_trigger/config.tab.*" -or \
        -wholename "./tools/ola_trigger/lex.yy.cpp" \) | xargs)
  if [[ $? -ne 0 ]]; then
    exit 1;
  fi;
elif [[ $TASK = 'check-licences' ]]; then
  # check licences only if it is the requested task
  autoreconf -i;
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for licence checking to run against
  make builtfiles
  ./scripts/enforce_licence.py
  if [[ $? -ne 0 ]]; then
    exit 1;
  fi;
elif [[ $TASK = 'spellintian' ]]; then
  # run spellintian only if it is the requested task, ignoring duplicate words
  autoreconf -i;
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for linting to run against
  make builtfiles
  spellingfiles=$(find ./ -type f -and ! \( \
      $SPELLINGBLACKLIST \
      \) | xargs)
  # count the number of spellintian errors, ignoring duplicate words
  spellingerrors=$(zrun spellintian $spellingfiles 2>&1 | grep -v "\(duplicate word\)" | wc -l)
  if [[ $spellingerrors -ne 0 ]]; then
    # print the output for info
    zrun spellintian $spellingfiles | grep -v "\(duplicate word\)"
    echo "Found $spellingerrors spelling errors, ignoring duplicates"
    exit 1;
  else
    echo "Found $spellingerrors spelling errors, ignoring duplicates"
  fi;
elif [[ $TASK = 'spellintian-duplicates' ]]; then
  # run spellintian only if it is the requested task
  autoreconf -i;
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for linting to run against
  make builtfiles
  spellingfiles=$(find ./ -type f -and ! \( \
      $SPELLINGBLACKLIST \
      \) | xargs)
  # count the number of spellintian errors
  spellingerrors=$(zrun spellintian $spellingfiles 2>&1 | wc -l)
  if [[ $spellingerrors -ne 0 ]]; then
    # print the output for info
    zrun spellintian $spellingfiles
    echo "Found $spellingerrors spelling errors"
    exit 1;
  else
    echo "Found $spellingerrors spelling errors"
  fi;
elif [[ $TASK = 'doxygen' ]]; then
  # check doxygen only if it is the requested task
  autoreconf -i;
  # Doxygen is C++ only, so don't bother with RDM tests
  ./configure --enable-ja-rule --enable-e133;
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for Doxygen to run against
  make builtfiles
  # count the number of warnings
  warnings=$(make doxygen-doc 2>&1 >/dev/null | wc -l)
  if [[ $warnings -ne 0 ]]; then
    # print the output for info
    make doxygen-doc
    echo "Found $warnings doxygen warnings"
    exit 1;
  else
    echo "Found $warnings doxygen warnings"
  fi;
elif [[ $TASK = 'coverage' ]]; then
  # Compile with coverage for coveralls
  autoreconf -i;
  # Coverage is C++ only, so don't bother with RDM tests
  ./configure --enable-gcov --enable-ja-rule --enable-e133;
  make;
  make check;
elif [[ $TASK = 'coverity' ]]; then
  # Run Coverity Scan unless token is zero length
  # The Coverity Scan script also relies on a number of other COVERITY_SCAN_
  # variables set in .travis.yml
  if [[ ${#COVERITY_SCAN_TOKEN} -ne 0 ]]; then
    curl -s $COVERITY_SCAN_BUILD_URL | bash
  else
    echo "Skipping Coverity Scan as no token found, probably a Pull Request"
  fi;
elif [[ $TASK = 'jshint' ]]; then
  cd ./javascript/new-src;
  npm install;
  grunt test
elif [[ $TASK = 'flake8' ]]; then
  autoreconf -i;
  ./configure --enable-rdm-tests
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for flake8 to run against
  make builtfiles
  flake8 --max-line-length 80 --exclude *_pb2.py,.git,__pycache --ignore E111,E114,E121,E127,E129 data/rdm include/ola python scripts tools/ola_mon tools/rdm
else
  # Otherwise compile and check as normal
  export DISTCHECK_CONFIGURE_FLAGS='--enable-rdm-tests --enable-ja-rule --enable-e133'
  autoreconf -i;
  ./configure $DISTCHECK_CONFIGURE_FLAGS;
  make distcheck;
  make dist;
  tarball=$(ls -Ut ola*.tar.gz | head -1)
  tar -zxf $tarball;
  tarball_root=$(echo $tarball | sed 's/.tar.gz$//')
  ./scripts/verify_trees.py ./ $tarball_root
fi
