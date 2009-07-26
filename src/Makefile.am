
AM_CXXFLAGS = -Wall -Werror $(libola_CFLAGS)
AM_LDFLAGS =  $(libola_LIBS)

if HAVE_NCURSES
NCURSES_PROGS = ola_dmxconsole ola_dmxmonitor
ola_dmxconsole_SOURCES = ola-dmxconsole.cpp
ola_dmxconsole_LDADD = @LIBS@ -lcurses
ola_dmxmonitor_SOURCES = ola-dmxmonitor.cpp
ola_dmxmonitor_LDADD = @LIBS@ -lcurses
endif

bin_PROGRAMS = ola_dev_info ola_usbpro ola_artnet $(NCURSES_PROGS)
ola_dev_info_SOURCES = ola-client.cpp
ola_usbpro_SOURCES = ola-usbpro.cpp
ola_usbpro_LDADD = @LIBS@ -lolausbproconf libolaconfig.la
ola_artnet_SOURCES = ola-artnet.cpp
ola_artnet_LDADD = @LIBS@ -lolaartnetconf libolaconfig.la

noinst_LTLIBRARIES = libolaconfig.la
libolaconfig_la_SOURCES = OlaConfigurator.cpp

EXTRA_DIST = OlaConfigurator.h

install-exec-hook:
	$(LN_S) -f $(bindir)/ola_dev_info $(bindir)/ola_plugin_info
	$(LN_S) -f $(bindir)/ola_dev_info $(bindir)/ola_uni_info
	$(LN_S) -f $(bindir)/ola_dev_info $(bindir)/ola_patch
	$(LN_S) -f $(bindir)/ola_dev_info $(bindir)/ola_uni_name
	$(LN_S) -f $(bindir)/ola_dev_info $(bindir)/ola_uni_merge
	$(LN_S) -f $(bindir)/ola_dev_info $(bindir)/ola_set_dmx
