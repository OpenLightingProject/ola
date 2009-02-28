
AM_CXXFLAGS = -Wall $(liblla_CFLAGS)
AM_LDFLAGS =  -LLIBDIR $(liblla_LIBS)

if HAVE_NCURSES
NCURSES_PROGS = lla_dmxconsole lla_dmxmonitor
lla_dmxconsole_SOURCES = lla-dmxconsole.cpp
lla_dmxconsole_LDADD = @LIBS@ -lcurses
lla_dmxmonitor_SOURCES = lla-dmxmonitor.cpp
lla_dmxmonitor_LDADD = @LIBS@ -lcurses
endif

bin_PROGRAMS = lla_dev_info lla_usbpro lla_artnet $(NCURSES_PROGS)
lla_dev_info_SOURCES = lla-client.cpp
lla_usbpro_SOURCES = lla-usbpro.cpp
lla_usbpro_LDADD = @LIBS@ -lllausbproconf libllaconfig.la
lla_artnet_SOURCES = lla-artnet.cpp
lla_artnet_LDADD = @LIBS@ -lllaartnetconf libllaconfig.la

noinst_LTLIBRARIES = libllaconfig.la
libllaconfig_la_SOURCES = LlaConfigurator.cpp

EXTRA_DIST = LlaConfigurator.h

install-exec-hook:
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_plugin_info
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_uni_info
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_patch
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_uni_name
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_uni_merge
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_set_dmx
