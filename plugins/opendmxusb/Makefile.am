lib_LTLIBRARIES = libllaopendmx.la

libdir = $(plugindir)

INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W


libllaopendmx_la_SOURCES =  opendmxplugin.cpp opendmxdevice.cpp opendmxport.cpp opendmxthread.cpp
libllaopendmx_la_LIBADD = @LIBS@  -lpthread
