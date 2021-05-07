include olad/www/Makefile.mk

dist_noinst_DATA += olad/testdata/test_preferences.conf

# HEADERS
##################################################
oladinclude_HEADERS += olad/OlaDaemon.h olad/OlaServer.h

# LIBRARIES
##################################################
ola_server_sources = \
    olad/ClientBroker.cpp \
    olad/ClientBroker.h \
    olad/DiscoveryAgent.cpp \
    olad/DiscoveryAgent.h \
    olad/DynamicPluginLoader.cpp \
    olad/DynamicPluginLoader.h \
    olad/HttpServerActions.h \
    olad/OlaServerServiceImpl.cpp \
    olad/OlaServerServiceImpl.h \
    olad/OladHTTPServer.h \
    olad/PluginLoader.h \
    olad/PluginManager.cpp \
    olad/PluginManager.h \
    olad/RDMHTTPModule.h
ola_server_additional_libs =

if HAVE_DNSSD
ola_server_sources += olad/BonjourDiscoveryAgent.h \
                      olad/BonjourDiscoveryAgent.cpp
endif

if HAVE_AVAHI
ola_server_sources += olad/AvahiDiscoveryAgent.h olad/AvahiDiscoveryAgent.cpp
ola_server_additional_libs += $(avahi_LIBS)
endif

if HAVE_LIBMICROHTTPD
ola_server_sources += olad/HttpServerActions.cpp \
                      olad/OladHTTPServer.cpp \
                      olad/RDMHTTPModule.cpp
ola_server_additional_libs += common/http/libolahttp.la
endif

# lib olaserver
lib_LTLIBRARIES += olad/libolaserver.la

olad_libolaserver_la_SOURCES = $(ola_server_sources) \
                               olad/OlaServer.cpp \
                               olad/OlaDaemon.cpp
olad_libolaserver_la_CXXFLAGS = $(COMMON_PROTOBUF_CXXFLAGS) \
                                -DHTTP_DATA_DIR=\"${www_datadir}\"
olad_libolaserver_la_LIBADD = $(PLUGIN_LIBS) \
                              common/libolacommon.la \
                              common/web/libolaweb.la \
                              ola/libola.la \
                              olad/plugin_api/libolaserverplugininterface.la \
                              $(ola_server_additional_libs)
# Simon: I'm not too sure about this but it seems that because PLUGIN_LIBS is
# determined at configure time, we need to add them here.
EXTRA_olad_libolaserver_la_DEPENDENCIES = $(PLUGIN_LIBS)

# PROGRAMS
##################################################
bin_PROGRAMS += olad/olad
olad_olad_SOURCES = olad/Olad.cpp

if SUPPORTS_RDYNAMIC
olad_olad_LDFLAGS = -rdynamic
endif
olad_olad_LDADD = olad/libolaserver.la \
                  common/libolacommon.la \
                  ola/libola.la

# TESTS
##################################################
test_programs += \
    olad/OlaTester

COMMON_OLAD_TEST_LDADD = $(COMMON_TESTING_LIBS) $(libprotobuf_LIBS) \
                         olad/plugin_api/libolaserverplugininterface.la \
                         olad/libolaserver.la \
                         common/libolacommon.la

olad_OlaTester_SOURCES = \
    olad/PluginManagerTest.cpp \
    olad/OlaServerServiceImplTest.cpp
olad_OlaTester_CXXFLAGS = $(COMMON_TESTING_PROTOBUF_FLAGS)
olad_OlaTester_LDADD = $(COMMON_OLAD_TEST_LDADD)

CLEANFILES += olad/ola-output.conf
