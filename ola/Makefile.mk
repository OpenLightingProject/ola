# HEADERS
##################################################
# These headers are deprecated and will be removed 6 months after the 0.9.0
# release.
DEPRECATED = \
    ola/OlaCallbackClient.h \
    ola/OlaDevice.h \
    ola/StreamingClient.h

pkginclude_HEADERS += \
    ola/AutoStart.h \
    ola/OlaClientWrapper.h \
    $(DEPRECATED)

# LIBRARIES
##################################################
lib_LTLIBRARIES += ola/libola.la
ola_libola_la_SOURCES = \
    ola/AutoStart.cpp \
    ola/ClientRDMAPIShim.cpp \
    ola/ClientTypesFactory.h \
    ola/ClientTypesFactory.cpp \
    ola/Module.cpp \
    ola/OlaCallbackClient.cpp \
    ola/OlaClient.cpp \
    ola/OlaClientCore.h \
    ola/OlaClientCore.cpp \
    ola/OlaClientWrapper.cpp \
    ola/StreamingClient.cpp
ola_libola_la_LDFLAGS = -version-info 1:1:0
ola_libola_la_LIBADD = common/libolacommon.la

# TESTS
##################################################
test_programs += ola/OlaClientTester

ola_OlaClientTester_SOURCES = ola/StreamingClientTest.cpp
ola_OlaClientTester_CXXFLAGS = $(COMMON_TESTING_FLAGS)
ola_OlaClientTester_LDADD = $(COMMON_TESTING_LIBS) \
                            $(PLUGIN_LIBS) \
                            common/libolacommon.la \
                            olad/libolaserver.la \
                            ola/libola.la
