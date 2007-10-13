lib_LTLIBRARIES = liblladummy.la

libdir = $(plugindir)

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W


liblladummy_la_SOURCES =  DummyPlugin.cpp DummyPlugin.h DummyPort.cpp\
							DummyPort.h DummyDevice.cpp DummyDevice.h

liblladummy_la_LIBADD = @LIBS@


