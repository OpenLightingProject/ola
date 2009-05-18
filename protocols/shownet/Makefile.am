
auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                        $(auxdir)/mkinstalldirs

SUBDIRS = shownet

EXTRA_DIST = reconf libshownet.pc.in Doxyfile

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libshownet.pc

ACLOCAL                 = aclocal -I $(auxdir)
