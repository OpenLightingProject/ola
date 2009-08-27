libdir = $(plugindir)

#EXTRA_DIST =

if HAVE_ACN

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libolae131.la
libolae131_la_SOURCES = E131Plugin.cpp E131Plugin.h E131Device.cpp \
                        OlaNetServer.cpp OlaNetServer.h \
                        E131Device.h E131Port.cpp E131Port.h
libolae131_la_LIBADD = -lacne131 ${libacn_LIBS}

endif
