
auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                        $(auxdir)/mkinstalldirs

AM_CXXFLAGS = -Wall -Werror -Wformat -W

SUBDIRS = include common ola plugins libltdl olad python tools

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libola.pc

ACLOCAL         = aclocal -I $(auxdir)
ACLOCAL_AMFLAGS = -I config
