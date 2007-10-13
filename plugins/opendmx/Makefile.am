lib_LTLIBRARIES = libllaopendmx.la

libdir = $(plugindir)

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CFLAGS = -Wall -Werror -Wformat -W


libllaopendmx_la_SOURCES =  OpenDmxPlugin.cpp OpenDmxPlugin.h OpenDmxDevice.cpp\
							OpenDmxDevice.h OpenDmxPort.cpp OpenDmxPort.h \
							OpenDmxThread.cpp OpenDmxThread.h
libllaopendmx_la_LIBADD = @LIBS@  -lpthread
