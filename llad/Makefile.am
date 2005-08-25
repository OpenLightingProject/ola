AM_CFLAGS = -Wall -Werror 

bin_PROGRAMS = llad

INCLUDES = @LLAD_DEFINES@

llad_SOURCES = main.cpp devicemanager.cpp  pluginadaptor.cpp\
				llad.cpp network.cpp  device.cpp port.cpp\
				pluginloader.cpp universe.cpp logger.cpp


llad_LDADD = @LIBS@ -ldl
llad_LDFLAGS = -rdynamic

