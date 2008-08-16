
libdir = $(plugindir)

if HAVE_ACN

INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include
AM_CXXFLAGS = -Wall -Werror -Wformat -W

lib_LTLIBRARIES = libllae131.la
libllae131_la_SOURCES = E131Plugin.cpp E131Plugin.h E131Device.cpp \
                        LlaNetServer.cpp LlaNetServer.h \
                        E131Device.h E131Port.cpp E131Port.h

libllae131_la_LIBADD = -lacne131 ${libacn_LIBS}

endif
