
bin_PROGRAMS = llad

INCLUDES = -I$(top_builddir)/include @LLAD_DEFINES@

llad_SOURCES = main.cpp devicemanager.cpp pluginadaptor.cpp\
				llad.cpp network.cpp device.cpp port.cpp\
				pluginloader.cpp universe.cpp logger.cpp\
				client.cpp preferences.cpp UniverseStore.cpp

EXTRA_DIST = devicemanager.h llad.h network.h pluginloader.h client.h UniverseStore.h

llad_CXXFLAGS = -Wall -Werror  -Wformat -W
llad_LDADD = @LIBS@ -ldl
llad_LDFLAGS = -rdynamic

