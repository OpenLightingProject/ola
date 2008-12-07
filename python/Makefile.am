
# python wrappers - experiemental !
#
#
pkgpyexecdir = @pythondir@
pkgpythondir = @pythondir@

EXTRA_DIST = LlaClient.i

if BUILD_PYTHON_LIBS

SWIG_SOURCES = LlaClient.i

nodist_pkgpython_PYTHON = lla.py
pkgpyexec_LTLIBRARIES = _lla.la
nodist__lla_la_SOURCES = LlaClient_wrap.cxx $(SWIG_SOURCES)
_lla_la_CPPFLAGS = $(SWIG_PYTHON_CPPFLAGS)
_lla_la_LDFLAGS = -module
_lla_la_LIBADD = ../lla/liblla.la


LlaClient_wrap.cxx lla.py: $(SWIG_SOURCES)
	$(SWIG) -c++ $(SWIG_PYTHON_OPT) -I$(top_srcdir)/src  $(SWIG_SOURCES)

clean-local:
	rm *.{h,cxx,py}


all-local:
	if [ ! -e _lla.so ]; then ln -s .libs/_lla.so _lla.so; fi

endif
