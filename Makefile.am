auxdir                  = @ac_aux_dir@
AUX_DIST                = $(auxdir)/install-sh $(auxdir)/missing \
                          $(auxdir)/mkinstalldirs

SUBDIRS = include data common ola plugins olad python tools examples debian

EXTRA_DIST = mac_build.sh README.debian README.rpm ola.spec

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libola.pc libolaserver.pc

ACLOCAL         = aclocal -I $(auxdir)
ACLOCAL_AMFLAGS = -I config
