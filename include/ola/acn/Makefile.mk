olaacnincludedir = $(pkgincludedir)/acn/
olaacninclude_HEADERS =

if INSTALL_ACN
olaacninclude_HEADERS += \
    include/ola/acn/ACNPort.h \
    include/ola/acn/ACNVectors.h \
    include/ola/acn/CID.h
endif
