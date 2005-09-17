
auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                        $(auxdir)/mkinstalldirs

SUBDIRS = lla llad plugins

EXTRA_DIST = reconf 

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblla.pc

ACLOCAL                 = aclocal -I $(auxdir)

