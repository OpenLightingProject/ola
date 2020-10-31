#!/bin/bash
PYTHON_EXE=python2
CPP_LINT_SCRIPT="cpplint.py"
if ! command -v cpplint.py &> /dev/null
then
    CPP_LINT_SCRIPT="$PYTHON_EXE cpplint.py"
fi

CPP_LINT_FILTER="-legal/copyright,-readability/streams,-runtime/arrays"
CPP_LINT_FILES=$(find ./ \( -name "*.h" -or -name "*.cpp" \) -and ! \( \
        -wholename "./common/protocol/Ola.pb.*" -or \
        -wholename "./common/rpc/Rpc.pb.*" -or \
        -wholename "./common/rpc/TestService.pb.*" -or \
        -wholename "./common/rdm/Pids.pb.*" -or \
        -wholename "./config.h" -or \
        -wholename "./plugins/*/messages/*ConfigMessages.pb.*" -or \
        -wholename "./tools/ola_trigger/config.tab.*" -or \
        -wholename "./tools/ola_trigger/lex.yy.cpp" \) | xargs)

"$CPP_LINT_SCRIPT" --filter="$CPP_LINT_FILTER" $CPP_LINT_FILES
