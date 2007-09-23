
bin_PROGRAMS = llad llad_test

INCLUDES = -I$(top_builddir)/include @LLAD_DEFINES@

llad_SOURCES = main.cpp devicemanager.cpp pluginadaptor.cpp\
				llad.cpp network.cpp plugin.cpp device.cpp port.cpp\
				DlOpenPluginLoader.cpp universe.cpp logger.cpp\
				client.cpp preferences.cpp UniverseStore.cpp

EXTRA_DIST = devicemanager.h llad.h network.h PluginLoader.h client.h UniverseStore.h \
			  DlOpenPluginLoader.h DynamicPluginLoader.h

llad_CXXFLAGS = -Wall -Werror  -Wformat -W
llad_LDADD = @LIBS@ -ldl
llad_LDFLAGS = -rdynamic


llad_test_SOURCES = main_test.cpp devicemanager.cpp pluginadaptor.cpp\
				llad.cpp network.cpp plugin.cpp device.cpp port.cpp\
				DynamicPluginLoader.cpp universe.cpp logger.cpp\
				client.cpp preferences.cpp UniverseStore.cpp

#llad_test_CXXFLAGS = -Wall -Werror  -Wformat -W
llad_test_LDADD = @LIBS@ ${PLUGIN_LIBS}
llad_test_CXXFLAGS = -g 
#-llladummy -lllaespnet \
#					-lllaopendmx -lllapathport -lllasandnet -lllashownet -lllastageprofi \
#					-lllausbpro
#llad_test_LDADD = @LIBS@
#llad_test_LDFLAGS = -rdynamic
