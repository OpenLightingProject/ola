include $(top_srcdir)/common.mk
libdir = $(third_party_plugindir)

EXTRA_DIST = DummyPlugin.h DummyDevice.h DummyPort.h

lib_LTLIBRARIES = liboladummy.la
liboladummy_la_SOURCES =  DummyPlugin.cpp DummyDevice.cpp DummyPort.cpp
liboladummy_la_LIBADD = @LIBS@
