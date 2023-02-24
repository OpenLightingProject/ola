olabaseincludedir = $(pkgincludedir)/base/
olabaseinclude_HEADERS = \
    include/ola/base/Array.h \
    include/ola/base/Credentials.h \
    include/ola/base/Env.h \
    include/ola/base/Flags.h \
    include/ola/base/FlagsPrivate.h \
    include/ola/base/Init.h \
    include/ola/base/Macro.h \
    include/ola/base/SysExits.h

nodist_olabaseinclude_HEADERS = \
    include/ola/base/Version.h
