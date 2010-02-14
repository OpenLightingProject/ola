auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                          $(auxdir)/mkinstalldirs

SUBDIRS =

if USE_LTDL
  SUBDIRS += libltdl
endif

SUBDIRS += include common ola plugins olad python tools

EXTRA_DIST = README.rpm ola.spec

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libola.pc

ACLOCAL         = aclocal -I $(auxdir)
ACLOCAL_AMFLAGS = -I config
