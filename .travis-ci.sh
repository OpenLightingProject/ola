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
elif [[ $TASK = 'make check' ]]
  # Otherwise compile and check as we used to
  autoreconf -i && ./configure --enable-rdm-tests && make && make check
else
  # Otherwise compile and check as normal
  autoreconf -i && ./configure && make distcheck
fi
