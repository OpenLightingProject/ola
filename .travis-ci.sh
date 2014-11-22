#!/bin/bash

CPP_LINT_URL="http://google-styleguide.googlecode.com/svn/trunk/cpplint/cpplint.py";

if [[ $TASK = 'lint' ]]; then
  # run the lint tool only if it is the requested task
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
  autoreconf -i && ./configure
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
  autoreconf -i && ./configure --enable-gcov && make distcheck
else
  # Otherwise compile and check as normal
  autoreconf -i && ./configure && make distcheck
fi
