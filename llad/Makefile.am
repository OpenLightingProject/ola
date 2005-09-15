AM_CFLAGS = -Wall -Werror 

bin_PROGRAMS = llad

INCLUDES = @LLAD_DEFINES@

llad_SOURCES = main.cpp devicemanager.cpp devicemanager.h  pluginadaptor.cpp\
				llad.cpp llad.h network.cpp network.h device.cpp port.cpp\
				pluginloader.cpp pluginloader.h universe.cpp logger.cpp\
				client.cpp client.h preferences.cpp


llad_LDADD = @LIBS@ -ldl
llad_LDFLAGS = -rdynamic

