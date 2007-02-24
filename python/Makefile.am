
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

# python wrappers - experiemental !
#if HAVE_SWIG

SWIG_SOURCES = LlaClient.i

pkgpython_PYTHON = lla.py __init__.py
pkgpyexec_LTLIBRARIES = _lla.la
_lla_la_SOURCES = LlaClient_wrap.cxx $(SWIG_SOURCES)
_lla_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS)
_lla_la_LDFLAGS = -module
_lla_la_LIBADD = ../lla/liblla.la

#pythonSitePackagesdir = $(prefix)/python2.4/site-packages
#pythonSitePackages_LTLIBRARIES = _lla.la
#pythonSitePackages_SCRIPTS = lla.py

#_lla_la_SOURCES = LlaClient_wrap.cxx
#_lla_la_CPPFLAGS =  -I/usr/include/python2.4
#_lla_la_LDFLAGS = -module -avoid-version
#_lla_la_LIBADD =  liblla.la

LlaClient_wrap.cxx: $(SWIG_SOURCES)
	$(SWIG) -c++ $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src  LlaClient.i

all-local:
	if [ ! -e _lla.so ]; then ln -s .libs/_lla.so _lla.so; fi

#endif

