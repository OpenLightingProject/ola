libdir = $(plugindir)

EXTRA_DIST = OpenDmxPlugin.h OpenDmxDevice.h OpenDmxPort.h OpenDmxThread.h

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libolaopendmx.la
libolaopendmx_la_SOURCES =  OpenDmxPlugin.cpp OpenDmxDevice.cpp \
                            OpenDmxThread.cpp
libolaopendmx_la_LIBADD = @LIBS@  -lpthread
