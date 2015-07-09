#!/bin/bash

# This script is triggered from the script section of .travis.yml
# It runs the appropriate commands depending on the task requested.

CPP_LINT_URL="http://google-styleguide.googlecode.com/svn/trunk/cpplint/cpplint.py";

COVERITY_SCAN_BUILD_URL="https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh"

if [[ $TASK = 'lint' ]]; then
  # run the lint tool only if it is the requested task
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
    $(find ./ -name "*.h" -or -name "*.cpp" | xargs)
  if [[ $? -ne 0 ]]; then
    exit 1;
  fi;
elif [[ $TASK = 'check-licences' ]]; then
  # check licences only if it is the requested task
  ./scripts/enforce_licence.py
  if [[ $? -ne 0 ]]; then
    exit 1;
  fi;
elif [[ $TASK = 'doxygen' ]]; then
  # check doxygen only if it is the requested task
  autoreconf -i && ./configure --enable-ja-rule
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
  autoreconf -i && ./configure --enable-ja-rule --enable-gcov && make && make check
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
else
  # Otherwise compile and check as normal
  autoreconf -i;
  ./configure --enable-rdm-tests --enable-ja-rule;
  make distcheck DISTCHECK_CONFIGURE_FLAGS='--enable-rdm-tests --enable-ja-rule';
  make dist;
  tarball=$(ls -Ut ola*.tar.gz | head -1)
  tar -zxf $tarball;
  tarball_root=$(echo $tarball | sed 's/.tar.gz$//')
  ./scripts/verify_trees.py ./ $tarball_root
fi
