lib_LTLIBRARIES = liblladummy.la

libdir = $(plugindir)

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W


liblladummy_la_SOURCES =  dummyplugin.cpp dummyplugin.h dummyport.cpp\
							dummyport.h dummydevice.cpp dummydevice.h

liblladummy_la_LIBADD = @LIBS@  


