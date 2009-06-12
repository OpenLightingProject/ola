
auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                        $(auxdir)/mkinstalldirs

AM_CXXFLAGS = -Wall -Werror -Wformat -W

SUBDIRS = common lla plugins libltdl llad include python tools

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblla.pc

ACLOCAL         = aclocal -I $(auxdir)
ACLOCAL_AMFLAGS = -I config
