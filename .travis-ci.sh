#!/bin/bash

CPP_LINT_URL="http://google-styleguide.googlecode.com/svn/trunk/cpplint/cpplint.py";

# run the lint tool only if this is the gcc build
if [[ $CC = 'gcc' ]]; then
  wget -O cpplint.py $CPP_LINT_URL;
  chmod u+x cpplint.py;
  cpplint.py \
    --filter=-legal/copyright,-readability/streams,-runtime/arrays \
    $(find ./ -name "*.h" -or -name "*.cpp" | xargs)
  if [[ $? -ne 0 ]]; then
    exit 1;
  fi;
fi

autoreconf -i && ./configure --enable-rdm-tests && make && make check
