lib_LTLIBRARIES = libllaopendmx.la

libdir = $(plugindir)

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CFLAGS = -Wall -Werror -Wformat -W


libllaopendmx_la_SOURCES =  opendmxplugin.cpp opendmxplugin.h opendmxdevice.cpp\
							opendmxdevice.h opendmxport.cpp opendmxport.h \
							opendmxthread.cpp opendmxthread.h
libllaopendmx_la_LIBADD = @LIBS@  -lpthread
