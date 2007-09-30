
AM_CXXFLAGS = -Wall -Werror $(liblla_CFLAGS)
AM_LDFLAGS =  -LLIBDIR $(liblla_LIBS)

if HAVE_NCURSES
NCURSES_PROGS = lla_dmxconsole lla_dmxmonitor
lla_dmxconsole_SOURCES = lla-dmxconsole.cpp
lla_dmxconsole_LDADD = @LIBS@ -lcurses
lla_dmxmonitor_SOURCES = lla-dmxmonitor.cpp
lla_dmxmonitor_LDADD = @LIBS@ -lcurses
endif

bin_PROGRAMS = lla_patch lla_dev_info lla_usbpro $(NCURSES_PROGS)
lla_patch_SOURCES = lla-patch.cpp
lla_dev_info_SOURCES = lla-client.cpp
lla_usbpro_SOURCES = lla-usbpro.cpp
lla_usbpro_LDADD = @LIBS@ -lllausbproconf

install-exec-hook:
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_plugin_info
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_uni_info
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_uni_name
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_uni_merge
	$(LN_S) -f $(bindir)/lla_dev_info $(bindir)/lla_set_dmx
