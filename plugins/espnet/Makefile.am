
libdir = $(plugindir)

if HAVE_ESPNET

lib_LTLIBRARIES = libllaespnet.la

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

libllaespnet_la_SOURCES = EspNetPlugin.cpp EspNetPlugin.h EspNetDevice.cpp\
						spNetDevice.h EspNetPort.cpp EspNetPort.h common.h
libllaespnet_la_LIBADD = @LIBS@  -lespnet

endif
