#!/bin/bash

# This script is triggered from the script section of .travis.yml
# It runs the appropriate commands depending on the task requested.

set -e

CPP_LINT_URL="https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py";

COVERITY_SCAN_BUILD_URL="https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh"

PYCHECKER_BLACKLIST="threading,unittest,cmd,optparse,google,google.protobuf,ssl,fftpack,lapack_lite,mtrand"

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

if [[ $TASK = 'lint' ]]; then
  # run the lint tool only if it is the requested task
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for linting to run against
  travis_fold start "make_builtfiles"
  make builtfiles VERBOSE=1;
  travis_fold end "make_builtfiles"
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
  # run the cpplint tool, fetching it if necessary
  make cpplint VERBOSE=1
elif [[ $TASK = 'check-licences' ]]; then
  # check licences only if it is the requested task
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for licence checking to run against
  travis_fold start "make_builtfiles"
  make builtfiles VERBOSE=1;
  travis_fold end "make_builtfiles"
  ./scripts/enforce_licence.py
  if [[ $? -ne 0 ]]; then
    exit 1;
  fi;
elif [[ $TASK = 'spellintian' ]]; then
  # run spellintian only if it is the requested task, ignoring duplicate words
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for spellintian to run against
  travis_fold start "make_builtfiles"
  make builtfiles VERBOSE=1;
  travis_fold end "make_builtfiles"
  spellingfiles=$(eval "find ./ -type f -and ! \( \
      $SPELLINGBLACKLIST \
      \) | xargs")
  # count the number of spellintian errors, ignoring duplicate words
  spellingerrors=$(zrun spellintian $spellingfiles 2>&1 | grep -v "\(duplicate word\)" | wc -l)
  if [[ $spellingerrors -ne 0 ]]; then
    # print the output for info
    zrun spellintian $spellingfiles | grep -v "\(duplicate word\)"
    echo "Found $spellingerrors spelling errors via spellintian, ignoring duplicates"
    exit 1;
  else
    echo "Found $spellingerrors spelling errors via spellintian, ignoring duplicates"
  fi;
elif [[ $TASK = 'spellintian-duplicates' ]]; then
  # run spellintian only if it is the requested task
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for spellintian to run against
  travis_fold start "make_builtfiles"
  make builtfiles VERBOSE=1;
  travis_fold end "make_builtfiles"
  spellingfiles=$(eval "find ./ -type f -and ! \( \
      $SPELLINGBLACKLIST \
      \) | xargs")
  # count the number of spellintian errors
  spellingerrors=$(zrun spellintian $spellingfiles 2>&1 | wc -l)
  if [[ $spellingerrors -ne 0 ]]; then
    # print the output for info
    zrun spellintian $spellingfiles
    echo "Found $spellingerrors spelling errors via spellintian"
    exit 1;
  else
    echo "Found $spellingerrors spelling errors via spellintian"
  fi;
elif [[ $TASK = 'codespell' ]]; then
  # run codespell only if it is the requested task
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure --enable-rdm-tests --enable-ja-rule --enable-e133;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for codespell to run against
  travis_fold start "make_builtfiles"
  make builtfiles VERBOSE=1;
  travis_fold end "make_builtfiles"
  spellingfiles=$(eval "find ./ -type f -and ! \( \
      $SPELLINGBLACKLIST \
      \) | xargs")
  # count the number of codespell errors
  spellingerrors=$(zrun codespell --check-filenames --check-hidden --quiet 2 --regex "[a-zA-Z0-9][\\-'a-zA-Z0-9]+[a-zA-Z0-9]" --exclude-file .codespellignorelines --ignore-words .codespellignorewords $spellingfiles 2>&1 | wc -l)
  if [[ $spellingerrors -ne 0 ]]; then
    # print the output for info, including the count
    zrun codespell --count --check-filenames --check-hidden --quiet 2 --regex "[a-zA-Z0-9][\\-'a-zA-Z0-9]+[a-zA-Z0-9]" --exclude-file .codespellignorelines --ignore-words .codespellignorewords $spellingfiles
    echo "Found $spellingerrors spelling errors via codespell"
    exit 1;
  else
    echo "Found $spellingerrors spelling errors via codespell"
  fi;
elif [[ $TASK = 'doxygen' ]]; then
  # check doxygen only if it is the requested task
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  # Doxygen is C++ only, so don't bother with RDM tests
  travis_fold start "configure"
  ./configure --enable-ja-rule --enable-e133;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for Doxygen to run against
  travis_fold start "make_builtfiles"
  make builtfiles VERBOSE=1;
  travis_fold end "make_builtfiles"
  # count the number of warnings
  warnings=$(make doxygen-doc 2>&1 >/dev/null | wc -l)
  if [[ $warnings -ne 0 ]]; then
    # print the output for info
    make doxygen-doc VERBOSE=1
    echo "Found $warnings doxygen warnings"
    exit 1;
  else
    echo "Found $warnings doxygen warnings"
  fi;
elif [[ $TASK = 'coverage' ]]; then
  # Compile with coverage for coveralls
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  # Coverage is C++ only, so don't bother with RDM tests
  travis_fold start "configure"
  ./configure --enable-gcov --enable-ja-rule --enable-e133;
  travis_fold end "configure"
  travis_fold start "make"
  make;
  travis_fold end "make"
  travis_fold start "make_check"
  make check VERBOSE=1;
  travis_fold end "make_check"
elif [[ $TASK = 'coverity' ]]; then
  # Run Coverity Scan unless token is zero length
  # The Coverity Scan script also relies on a number of other COVERITY_SCAN_
  # variables set in .travis.yml
  if [[ ${#COVERITY_SCAN_TOKEN} -ne 0 ]]; then
    curl -s $COVERITY_SCAN_BUILD_URL | bash
  else
    echo "Skipping Coverity Scan as no token found, probably a Pull Request"
  fi;
elif [[ $TASK = 'weblint' ]]; then
  cd ./javascript/new-src;
  travis_fold start "npm_install"
  npm --verbose install;
  travis_fold end "npm_install"
  grunt -v -d --stack test
elif [[ $TASK = 'flake8' ]]; then
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure --enable-rdm-tests;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for flake8 to run against
  travis_fold start "make_builtfiles"
  make builtfiles VERBOSE=1;
  travis_fold end "make_builtfiles"
  make flake8
elif [[ $TASK = 'pychecker' ]]; then
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure --enable-rdm-tests;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for pychecker to run against
  travis_fold start "make_builtfiles"
  make builtfiles;
  travis_fold end "make_builtfiles"
  PYTHONPATH=./python/:$PYTHONPATH
  export PYTHONPATH
  mkdir ./python/ola/testing/
  ln -s ./tools/rdm ./python/ola/testing/rdm
  travis_fold start "pychecker_a"
  pychecker --quiet --limit 500 --blacklist $PYCHECKER_BLACKLIST $(find ./ -name "*.py" -and \( -wholename "./data/*" -or -wholename "./include/*" -or -wholename "./scripts/*" -or -wholename "./python/examples/rdm_compare.py" -or -wholename "./python/ola/*" \) -and ! \( -name "*_pb2.py" -or -name "OlaClient.py" -or -name "OlaClientTest.py" -or -name "ola_candidate_ports.py" -or -wholename "./scripts/enforce_licence.py" -or -wholename "./python/ola/rpc/*" -or -wholename "./python/ola/ClientWrapper.py" -or -wholename "./python/ola/PidStore.py" -or -wholename "./python/ola/RDMAPI.py" -or -wholename "./python/ola/RDMTest.py" -or -wholename "./include/ola/gen_callbacks.py" \) | xargs)
  travis_fold end "pychecker_a"
  # More restricted checking for files that import files that break pychecker
  travis_fold start "pychecker_b"
  pychecker --quiet --limit 500 --blacklist $PYCHECKER_BLACKLIST --only $(find ./ -name "*.py" -and \( -wholename "./tools/rdm/ModelCollector.py" -or -wholename "./tools/rdm/DMXSender.py" -or -wholename "./tools/rdm/TestCategory.py" -or -wholename "./tools/rdm/TestState.py" -or -wholename "./tools/rdm/TimingStats.py" -or -wholename "./tools/rdm/list_rdm_tests.py" \) | xargs)
  travis_fold end "pychecker_b"
  # Even more restricted checking for files that import files that break pychecker and have unused parameters
  travis_fold start "pychecker_c"
  pychecker --quiet --limit 500 --blacklist $PYCHECKER_BLACKLIST --only --no-argsused $(find ./ -name "*.py" -and ! \( -name "*_pb2.py" -or -name "OlaClient.py" -or -name "ola_candidate_ports.py" -or -name "ola_universe_info.py" -or -name "rdm_snapshot.py" -or -name "ClientWrapper.py" -or -name "PidStore.py" -or -name "enforce_licence.py" -or -name "ola_mon.py" -or -name "TestLogger.py" -or -name "TestRunner.py" -or -name "rdm_model_collector.py" -or -name "rdm_responder_test.py" -or -name "rdm_test_server.py" -or -wholename "./include/ola/gen_callbacks.py" -or -wholename "./tools/rdm/ResponderTest.py" -or -wholename "./tools/rdm/TestDefinitions.py" -or -wholename "./tools/rdm/TestHelpers.py" -or -wholename "./tools/rdm/TestMixins.py" \) | xargs)
  travis_fold end "pychecker_c"
  # Special case checking for some python 3 compatibility workarounds
  travis_fold start "pychecker_d"
  pychecker --quiet --limit 500 --no-shadowbuiltin --no-noeffect ./include/ola/gen_callbacks.py
  travis_fold end "pychecker_d"
  # Special case checking for some python 3 compatibility workarounds that import files that break pychecker
  travis_fold start "pychecker_e"
  pychecker --quiet --limit 500 --only --no-shadowbuiltin --no-noeffect ./tools/rdm/TestHelpers.py
  travis_fold end "pychecker_e"
  # Extra special case checking for some python 3 compatibility workarounds that import files that break pychecker and have unused parameters
  travis_fold start "pychecker_f"
  pychecker --quiet --limit 500 --only --no-argsused --no-shadowbuiltin --no-noeffect ./tools/rdm/ResponderTest.py
  travis_fold end "pychecker_f"
elif [[ $TASK = 'pychecker-wip' ]]; then
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure --enable-rdm-tests;
  travis_fold end "configure"
  # the following is a bit of a hack to build the files normally built during
  # the build, so they are present for pychecker to run against
  travis_fold start "make_builtfiles"
  make builtfiles;
  travis_fold end "make_builtfiles"
  PYTHONPATH=./python/:$PYTHONPATH
  export PYTHONPATH
  mkdir ./python/ola/testing/
  ln -s ./tools/rdm ./python/ola/testing/rdm
  pychecker --quiet --limit 500 --blacklist $PYCHECKER_BLACKLIST $(find ./ -name "*.py" -and ! \( -name "*_pb2.py" -or -name "OlaClient.py" -or -name "ola_candidate_ports.py" \) | xargs)
else
  # Otherwise compile and check as normal
  # Env var name DISTCHECK_CONFIGURE_FLAGS must be used, see #1881 and #1883
  if [[ "$TRAVIS_OS_NAME" = "linux" ]]; then
    # Silence all deprecated declarations on Linux due to auto_ptr making the build log too long
    export DISTCHECK_CONFIGURE_FLAGS='--enable-rdm-tests --enable-java-libs --enable-ja-rule --enable-e133 CPPFLAGS=-Wno-deprecated-declarations'
  else
    export DISTCHECK_CONFIGURE_FLAGS='--enable-rdm-tests --enable-java-libs --enable-ja-rule --enable-e133'
  fi
  travis_fold start "autoreconf"
  autoreconf -i;
  travis_fold end "autoreconf"
  travis_fold start "configure"
  ./configure $DISTCHECK_CONFIGURE_FLAGS;
  travis_fold end "configure"
  travis_fold start "make_distcheck"
  make distcheck VERBOSE=1;
  travis_fold end "make_distcheck"
  travis_fold start "make_dist"
  make dist;
  travis_fold end "make_dist"
  travis_fold start "verify_trees"
  tarball=$(ls -Ut ola*.tar.gz | head -1)
  tar -zxf $tarball;
  tarball_root=$(echo $tarball | sed 's/.tar.gz$//')
  ./scripts/verify_trees.py ./ $tarball_root
  travis_fold end "verify_trees"
fi
