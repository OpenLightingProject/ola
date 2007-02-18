
auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                        $(auxdir)/mkinstalldirs

AM_CXXFLAGS = -Wall -Werror -Wformat -W

SUBDIRS = lla plugins llad include

EXTRA_DIST = reconf tools

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = liblla.pc 

ACLOCAL                 = aclocal -I $(auxdir)

