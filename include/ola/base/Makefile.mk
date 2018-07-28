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

if HAVE_STRERROR_R
olabaseinclude_HEADERS += include/ola/base/Strerror_r.h
endif

nodist_olabaseinclude_HEADERS = \
    include/ola/base/Version.h
